/*
 * Copyright (C) 2019 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InlineLine.h"

#if ENABLE(LAYOUT_FORMATTING_CONTEXT)

#include "InlineFormattingContext.h"
#include "TextUtil.h"
#include <wtf/IsoMallocInlines.h>

namespace WebCore {
namespace Layout {

WTF_MAKE_ISO_ALLOCATED_IMPL(Line);

Line::Run::Run(const InlineItem& inlineItem, const Display::Run& displayRun)
    : m_inlineItem(inlineItem)
    , m_displayRun(displayRun)
{
}

bool Line::Run::isWhitespace() const
{
    if (!isText())
        return false;
    return downcast<InlineTextItem>(m_inlineItem).isWhitespace();
}

bool Line::Run::canBeExtended() const
{
    if (!isText())
        return false;
    // Non-collapsed text runs can be merged into one continuous run.
    if (isVisuallyEmpty())
        return false;
    return !isCollapsed();
}

Line::Line(const InlineFormattingContext& inlineFormattingContext, const InitialConstraints& initialConstraints, Optional<TextAlignMode> horizontalAlignment, SkipAlignment skipAlignment)
    : m_inlineFormattingContext(inlineFormattingContext)
    , m_initialStrut(initialConstraints.heightAndBaseline ? initialConstraints.heightAndBaseline->strut : WTF::nullopt)
    , m_lineLogicalWidth(initialConstraints.availableLogicalWidth)
    , m_horizontalAlignment(horizontalAlignment)
    , m_skipAlignment(skipAlignment == SkipAlignment::Yes)
{
    ASSERT(m_skipAlignment || initialConstraints.heightAndBaseline);
    auto initialLineHeight = initialConstraints.heightAndBaseline ? initialConstraints.heightAndBaseline->height : LayoutUnit();
    auto initialBaselineOffset = initialConstraints.heightAndBaseline ? initialConstraints.heightAndBaseline->baselineOffset : LayoutUnit();
    auto lineRect = Display::Rect { initialConstraints.logicalTopLeft, { }, initialLineHeight };
    auto baseline = LineBox::Baseline { initialBaselineOffset, initialLineHeight - initialBaselineOffset };
    m_lineBox = LineBox { lineRect, baseline, initialBaselineOffset };
}

static bool isInlineContainerConsideredEmpty(const FormattingContext& formattingContext, const Box& layoutBox)
{
    // Note that this does not check whether the inline container has content. It simply checks if the container itself is considered empty.
    auto& boxGeometry = formattingContext.geometryForBox(layoutBox);
    return !(boxGeometry.horizontalBorder() || (boxGeometry.horizontalPadding() && boxGeometry.horizontalPadding().value()));
}

static bool shouldPreserveTrailingContent(const InlineTextItem& inlineTextItem)
{
    if (!inlineTextItem.isWhitespace())
        return true;
    auto whitespace = inlineTextItem.style().whiteSpace();
    return whitespace == WhiteSpace::Pre || whitespace == WhiteSpace::PreWrap;
}

static bool shouldPreserveLeadingContent(const InlineTextItem& inlineTextItem)
{
    if (!inlineTextItem.isWhitespace())
        return true;
    auto whitespace = inlineTextItem.style().whiteSpace();
    return whitespace == WhiteSpace::Pre || whitespace == WhiteSpace::PreWrap || whitespace == WhiteSpace::BreakSpaces;
}

bool Line::isVisuallyEmpty() const
{
    // FIXME: This should be cached instead -as the inline items are being added.
    // Return true for empty inline containers like <span></span>.
    auto& formattingContext = this->formattingContext();
    for (auto& run : m_runList) {
        if (run->isContainerStart()) {
            if (!isInlineContainerConsideredEmpty(formattingContext, run->layoutBox()))
                return false;
            continue;
        }
        if (run->isContainerEnd())
            continue;
        if (run->layoutBox().establishesFormattingContext()) {
            ASSERT(run->layoutBox().isInlineBlockBox());
            auto& boxGeometry = formattingContext.geometryForBox(run->layoutBox());
            if (!boxGeometry.width())
                continue;
            if (m_skipAlignment || boxGeometry.height())
                return false;
            continue;
        }
        if (!run->isText() || !run->isVisuallyEmpty())
            return false;
    }
    return true;
}

Line::RunList Line::close()
{
    removeTrailingTrimmableContent();
    // Join text runs together when possible.
    unsigned index = 1;
    while (index < m_runList.size()) {
        auto& previousRun = m_runList[index - 1];
        if (!previousRun->canBeExtended()) {
            ++index;
            continue;
        }
        auto& currentRun = m_runList[index];
        if (!currentRun->isText() || &currentRun->layoutBox() != &previousRun->layoutBox()) {
            // Do not merge runs from different boxes (<span>foo</span><span>bar</span>)
            // or within the same layout box but with preserved \n
            // (<span>text\n<span <- both the "text" and "\" belong to the same layout box)
            ++index;
            continue;
        }
        // Only text content can be extended atm.
        ASSERT(previousRun->isText());
        ASSERT(currentRun->isText());
        previousRun->expand(*currentRun);
        m_runList.remove(index);
    }

    if (!m_skipAlignment) {
        alignContentVertically();
        alignContentHorizontally();
    }

    return WTFMove(m_runList);
}

void Line::alignContentVertically()
{
    ASSERT(!m_skipAlignment);

    if (isVisuallyEmpty()) {
        m_lineBox.resetBaseline();
        m_lineBox.setLogicalHeight({ });
    }

    // Remove descent when all content is baseline aligned but none of them have descent.
    if (formattingContext().quirks().lineDescentNeedsCollapsing(m_runList)) {
        m_lineBox.shrinkVertically(m_lineBox.baseline().descent());
        m_lineBox.resetDescent();
    }

    auto& layoutState = this->layoutState();
    auto& formattingContext = this->formattingContext();
    for (auto& run : m_runList) {
        LayoutUnit logicalTop;
        auto& layoutBox = run->layoutBox();
        auto verticalAlign = layoutBox.style().verticalAlign();
        auto ascent = layoutBox.style().fontMetrics().ascent();

        switch (verticalAlign) {
        case VerticalAlign::Baseline:
            if (run->isForcedLineBreak() || run->isText())
                logicalTop = baselineOffset() - ascent;
            else if (run->isContainerStart()) {
                auto& boxGeometry = formattingContext.geometryForBox(layoutBox);
                logicalTop = baselineOffset() - ascent - boxGeometry.borderTop() - boxGeometry.paddingTop().valueOr(0);
            } else if (layoutBox.isInlineBlockBox() && layoutBox.establishesInlineFormattingContext()) {
                auto& formattingState = downcast<InlineFormattingState>(layoutState.establishedFormattingState(downcast<Container>(layoutBox)));
                // Spec makes us generate at least one line -even if it is empty.
                ASSERT(!formattingState.lineBoxes().isEmpty());
                auto inlineBlockBaselineOffset = formattingState.lineBoxes().last()->baselineOffset();
                // The inline-block's baseline offset is relative to its content box. Let's convert it relative to the margin box.
                //   inline-block
                //              \
                //           _______________ <- margin box
                //          |
                //          |  ____________  <- border box
                //          | |
                //          | |  _________  <- content box
                //          | | |   ^
                //          | | |   |  <- baseline offset
                //          | | |   |
                //     text | | |   v text
                //     -----|-|-|---------- <- baseline
                //
                auto& boxGeometry = formattingContext.geometryForBox(layoutBox);
                auto baselineOffsetFromMarginBox = boxGeometry.marginBefore() + boxGeometry.borderTop() + boxGeometry.paddingTop().valueOr(0) + inlineBlockBaselineOffset;
                logicalTop = baselineOffset() - baselineOffsetFromMarginBox;
            } else
                logicalTop = baselineOffset() - run->logicalRect().height();
            break;
        case VerticalAlign::Top:
            logicalTop = { };
            break;
        case VerticalAlign::Bottom:
            logicalTop = logicalBottom() - run->logicalRect().height();
            break;
        default:
            ASSERT_NOT_IMPLEMENTED_YET();
            break;
        }
        run->adjustLogicalTop(logicalTop);
        // Convert runs from relative to the line top/left to the formatting root's border box top/left.
        run->moveVertically(this->logicalTop());
        run->moveHorizontally(this->logicalLeft());
    }
}

void Line::alignContentHorizontally()
{
    ASSERT(!m_skipAlignment);

    auto adjustmentForAlignment = [&]() -> Optional<LayoutUnit> {
        switch (*m_horizontalAlignment) {
        case TextAlignMode::Left:
        case TextAlignMode::WebKitLeft:
        case TextAlignMode::Start:
            return { };
        case TextAlignMode::Right:
        case TextAlignMode::WebKitRight:
        case TextAlignMode::End:
            return std::max(availableWidth(), 0_lu);
        case TextAlignMode::Center:
        case TextAlignMode::WebKitCenter:
            return std::max(availableWidth() / 2, 0_lu);
        case TextAlignMode::Justify:
            ASSERT_NOT_REACHED();
            break;
        }
        ASSERT_NOT_REACHED();
        return { };
    };

    auto adjustment = adjustmentForAlignment();
    if (!adjustment)
        return;

    for (auto& run : m_runList)
        run->moveHorizontally(*adjustment);
    // FIXME: Find out if m_lineBox needs adjustmnent as well.
}

void Line::removeTrailingTrimmableContent()
{
    // Collapse trimmable trailing content
    LayoutUnit trimmableWidth;
    for (auto* trimmableRun : m_trimmableContent) {
        ASSERT(trimmableRun->isText());
        trimmableRun->setVisuallyIsEmpty();
        trimmableWidth += trimmableRun->logicalRect().width();
    }
    m_lineBox.shrinkHorizontally(trimmableWidth);
}

void Line::moveLogicalLeft(LayoutUnit delta)
{
    if (!delta)
        return;
    ASSERT(delta > 0);
    m_lineBox.moveHorizontally(delta);
    m_lineLogicalWidth -= delta;
}

void Line::moveLogicalRight(LayoutUnit delta)
{
    ASSERT(delta > 0);
    m_lineLogicalWidth -= delta;
}

LayoutUnit Line::trailingTrimmableWidth() const
{
    LayoutUnit trimmableWidth;
    for (auto* trimmableRun : m_trimmableContent) {
        ASSERT(!trimmableRun->isVisuallyEmpty());
        trimmableWidth += trimmableRun->logicalRect().width();
    }
    return trimmableWidth;
}

void Line::append(const InlineItem& inlineItem, LayoutUnit logicalWidth)
{
    if (inlineItem.isForcedLineBreak())
        return appendLineBreak(inlineItem);
    if (is<InlineTextItem>(inlineItem))
        return appendTextContent(downcast<InlineTextItem>(inlineItem), logicalWidth);
    if (inlineItem.isContainerStart())
        return appendInlineContainerStart(inlineItem, logicalWidth);
    if (inlineItem.isContainerEnd())
        return appendInlineContainerEnd(inlineItem, logicalWidth);
    if (inlineItem.layoutBox().replaced())
        return appendReplacedInlineBox(inlineItem, logicalWidth);
    appendNonReplacedInlineBox(inlineItem, logicalWidth);
}

void Line::appendNonBreakableSpace(const InlineItem& inlineItem, const Display::Rect& logicalRect)
{
    m_runList.append(makeUnique<Run>(inlineItem, Display::Run { inlineItem.style(), logicalRect }));
    m_lineBox.expandHorizontally(logicalRect.width());
}

void Line::appendInlineContainerStart(const InlineItem& inlineItem, LayoutUnit logicalWidth)
{
    // This is really just a placeholder to mark the start of the inline level container <span>.
    auto logicalRect = Display::Rect { 0, contentLogicalWidth(), logicalWidth, 0 };

    if (!m_skipAlignment) {
        adjustBaselineAndLineHeight(inlineItem);
        logicalRect.setHeight(inlineItemContentHeight(inlineItem));
    }
    appendNonBreakableSpace(inlineItem, logicalRect);
}

void Line::appendInlineContainerEnd(const InlineItem& inlineItem, LayoutUnit logicalWidth)
{
    // This is really just a placeholder to mark the end of the inline level container </span>.
    auto logicalRect = Display::Rect { 0, contentLogicalRight(), logicalWidth, m_skipAlignment ? LayoutUnit() : inlineItemContentHeight(inlineItem) };
    appendNonBreakableSpace(inlineItem, logicalRect);
}

void Line::appendTextContent(const InlineTextItem& inlineItem, LayoutUnit logicalWidth)
{
    auto isTrimmable = !shouldPreserveTrailingContent(inlineItem);
    if (!isTrimmable)
        m_trimmableContent.clear();

    auto willCollapseCompletely = [&] {
        // Empty run.
        if (!inlineItem.length()) {
            ASSERT(!logicalWidth);
            return true;
        }
        // Leading whitespace.
        if (m_runList.isEmpty())
            return !shouldPreserveLeadingContent(inlineItem);

        if (!inlineItem.isCollapsible())
            return false;
        // Check if the last item is collapsed as well.
        for (auto i = m_runList.size(); i--;) {
            auto& run = m_runList[i];
            if (run->isBox())
                return false;
            // When the previous text run is collapsed, this collapsible run collapses completely.
            if (run->isText())
                return run->isCollapsed();
            // Collapsing works across inline containers: "<span>  </span> " <- the trailing whitespace collapses completely.
            // Not that when the inline container has preserve whitespace style, "<span style="white-space: pre">  </span> " <- this whitespace stays around. 
            ASSERT(run->isContainerStart() || run->isContainerEnd());
        }
        return true;
    };

    auto logicalRect = Display::Rect { };
    logicalRect.setLeft(contentLogicalWidth());
    logicalRect.setWidth(logicalWidth);
    if (!m_skipAlignment) {
        adjustBaselineAndLineHeight(inlineItem);
        logicalRect.setHeight(inlineItemContentHeight(inlineItem));
    }

    auto collapseRun = inlineItem.isCollapsible();
    auto contentStart = inlineItem.start();
    auto contentLength =  collapseRun ? 1 : inlineItem.length();
    auto textContent = inlineItem.layoutBox().textContent().substring(contentStart, contentLength);
    auto lineItem = makeUnique<Run>(inlineItem, Display::Run { inlineItem.style(), logicalRect, Display::Run::TextContext { contentStart, contentLength, textContent } });

    auto isVisuallyEmpty = willCollapseCompletely();
    if (collapseRun)
        lineItem->setIsCollapsed();
    if (isVisuallyEmpty)
        lineItem->setVisuallyIsEmpty();
    else if (isTrimmable)
        m_trimmableContent.add(lineItem.get());

    m_runList.append(WTFMove(lineItem));
    // Collapsed line items don't contribute to the line width.
    if (!isVisuallyEmpty)
        m_lineBox.expandHorizontally(logicalWidth);
}

void Line::appendNonReplacedInlineBox(const InlineItem& inlineItem, LayoutUnit logicalWidth)
{
    auto& boxGeometry = formattingContext().geometryForBox(inlineItem.layoutBox());
    auto horizontalMargin = boxGeometry.horizontalMargin();
    auto logicalRect = Display::Rect { };

    logicalRect.setLeft(contentLogicalWidth() + horizontalMargin.start);
    logicalRect.setWidth(logicalWidth);
    if (!m_skipAlignment) {
        adjustBaselineAndLineHeight(inlineItem);
        auto runHeight = formattingContext().geometryForBox(inlineItem.layoutBox()).marginBoxHeight();
        logicalRect.setHeight(runHeight);
    }

    m_runList.append(makeUnique<Run>(inlineItem, Display::Run { inlineItem.style(), logicalRect }));
    m_lineBox.expandHorizontally(logicalWidth + horizontalMargin.start + horizontalMargin.end);
    m_trimmableContent.clear();
}

void Line::appendReplacedInlineBox(const InlineItem& inlineItem, LayoutUnit logicalWidth)
{
    ASSERT(inlineItem.layoutBox().isReplaced());
    // FIXME: Surely replaced boxes behave differently.
    appendNonReplacedInlineBox(inlineItem, logicalWidth);
    if (auto* replaced = inlineItem.layoutBox().replaced(); replaced && replaced->cachedImage())
        m_runList.last()->m_displayRun.setImage(*replaced->cachedImage());
}

void Line::appendLineBreak(const InlineItem& inlineItem)
{
    auto logicalRect = Display::Rect { };
    logicalRect.setLeft(contentLogicalWidth());
    logicalRect.setWidth({ });
    if (!m_skipAlignment) {
        adjustBaselineAndLineHeight(inlineItem);
        logicalRect.setHeight(logicalHeight());
    }
    m_runList.append(makeUnique<Run>(inlineItem, Display::Run { inlineItem.style(), logicalRect }));
}

void Line::adjustBaselineAndLineHeight(const InlineItem& inlineItem)
{
    ASSERT(!inlineItem.isContainerEnd());
    auto& layoutBox = inlineItem.layoutBox();
    auto& style = layoutBox.style();
    auto& baseline = m_lineBox.baseline();

    if (inlineItem.isContainerStart()) {
        // Inline containers stretch the line by their font size.
        // Vertical margins, paddings and borders don't contribute to the line height.
        auto& fontMetrics = style.fontMetrics();
        if (style.verticalAlign() == VerticalAlign::Baseline) {
            auto halfLeading = halfLeadingMetrics(fontMetrics, style.computedLineHeight());
            // Both halfleading ascent and descent could be negative (tall font vs. small line-height value)
            if (halfLeading.descent() > 0)
                m_lineBox.setDescentIfGreater(halfLeading.descent());
            if (halfLeading.ascent() > 0)
                m_lineBox.setAscentIfGreater(halfLeading.ascent());
            m_lineBox.setLogicalHeightIfGreater(baseline.height());
        } else
            m_lineBox.setLogicalHeightIfGreater(fontMetrics.height());
        return;
    }

    if (inlineItem.isText() || inlineItem.isForcedLineBreak()) {
        // For text content we set the baseline either through the initial strut (set by the formatting context root) or
        // through the inline container (start) -see above. Normally the text content itself does not stretch the line.
        if (!m_initialStrut)
            return;
        m_lineBox.setAscentIfGreater(m_initialStrut->ascent());
        m_lineBox.setDescentIfGreater(m_initialStrut->descent());
        m_lineBox.setLogicalHeightIfGreater(baseline.height());
        m_initialStrut = { };
        return;
    }

    if (inlineItem.isBox()) {
        auto& boxGeometry = formattingContext().geometryForBox(layoutBox);
        auto marginBoxHeight = boxGeometry.marginBoxHeight();

        switch (style.verticalAlign()) {
        case VerticalAlign::Baseline: {
            if (layoutBox.isInlineBlockBox() && layoutBox.establishesInlineFormattingContext()) {
                // Inline-blocks with inline content always have baselines.
                auto& formattingState = downcast<InlineFormattingState>(layoutState().establishedFormattingState(downcast<Container>(layoutBox)));
                // Spec makes us generate at least one line -even if it is empty.
                ASSERT(!formattingState.lineBoxes().isEmpty());
                auto& lastLineBox = *formattingState.lineBoxes().last();
                auto inlineBlockBaseline = lastLineBox.baseline();
                auto beforeHeight = boxGeometry.marginBefore() + boxGeometry.borderTop() + boxGeometry.paddingTop().valueOr(0);

                m_lineBox.setAscentIfGreater(inlineBlockBaseline.ascent());
                m_lineBox.setDescentIfGreater(inlineBlockBaseline.descent());
                m_lineBox.setBaselineOffsetIfGreater(beforeHeight + lastLineBox.baselineOffset());
                m_lineBox.setLogicalHeightIfGreater(marginBoxHeight);
            } else {
                // Non inline-block boxes sit on the baseline (including their bottom margin).
                m_lineBox.setAscentIfGreater(marginBoxHeight);
                // Ignore negative descent (yes, negative descent is a thing).
                m_lineBox.setLogicalHeightIfGreater(marginBoxHeight + std::max(LayoutUnit(), baseline.descent()));
            }
            break;
        }
        case VerticalAlign::Top:
            // Top align content never changes the baseline, it only pushes the bottom of the line further down.
            m_lineBox.setLogicalHeightIfGreater(marginBoxHeight);
            break;
        case VerticalAlign::Bottom: {
            // Bottom aligned, tall content pushes the baseline further down from the line top.
            auto lineLogicalHeight = m_lineBox.logicalHeight();
            if (marginBoxHeight > lineLogicalHeight) {
                m_lineBox.setLogicalHeightIfGreater(marginBoxHeight);
                m_lineBox.setBaselineOffsetIfGreater(m_lineBox.baselineOffset() + (marginBoxHeight - lineLogicalHeight));
            }
            break;
        }
        default:
            ASSERT_NOT_IMPLEMENTED_YET();
            break;
        }
        return;
    }
    ASSERT_NOT_REACHED();
}

LayoutUnit Line::inlineItemContentHeight(const InlineItem& inlineItem) const
{
    ASSERT(!m_skipAlignment);
    auto& fontMetrics = inlineItem.style().fontMetrics();
    if (inlineItem.isForcedLineBreak() || is<InlineTextItem>(inlineItem))
        return fontMetrics.height();

    auto& layoutBox = inlineItem.layoutBox();
    auto& boxGeometry = formattingContext().geometryForBox(layoutBox);

    if (layoutBox.replaced() || layoutBox.isFloatingPositioned())
        return boxGeometry.contentBoxHeight();

    if (inlineItem.isContainerStart() || inlineItem.isContainerEnd())
        return fontMetrics.height();

    // Non-replaced inline box (e.g. inline-block). It looks a bit misleading but their margin box is considered the content height here.
    return boxGeometry.marginBoxHeight();
}

LineBox::Baseline Line::halfLeadingMetrics(const FontMetrics& fontMetrics, LayoutUnit lineLogicalHeight)
{
    auto ascent = fontMetrics.ascent();
    auto descent = fontMetrics.descent();
    // 10.8.1 Leading and half-leading
    auto leading = lineLogicalHeight - (ascent + descent);
    // Inline tree is all integer based.
    auto adjustedAscent = std::max((ascent + leading / 2).floor(), 0);
    auto adjustedDescent = std::max((descent + leading / 2).ceil(), 0);
    return { adjustedAscent, adjustedDescent };
}

LayoutState& Line::layoutState() const
{ 
    return formattingContext().layoutState();
}

const InlineFormattingContext& Line::formattingContext() const
{
    return m_inlineFormattingContext;
} 

}
}

#endif
