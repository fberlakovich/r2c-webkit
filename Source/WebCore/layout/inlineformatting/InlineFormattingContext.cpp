/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
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
#include "InlineFormattingContext.h"

#if ENABLE(LAYOUT_FORMATTING_CONTEXT)

#include "FloatingState.h"
#include "InlineFormattingState.h"
#include "InlineLineBreaker.h"
#include "InlineRunProvider.h"
#include "LayoutBox.h"
#include "LayoutContainer.h"
#include "LayoutFormattingState.h"
#include "LayoutInlineBox.h"
#include "LayoutInlineContainer.h"
#include "Logging.h"
#include <wtf/IsoMallocInlines.h>
#include <wtf/text/TextStream.h>

namespace WebCore {
namespace Layout {

WTF_MAKE_ISO_ALLOCATED_IMPL(InlineFormattingContext);

InlineFormattingContext::InlineFormattingContext(const Box& formattingContextRoot, FormattingState& formattingState)
    : FormattingContext(formattingContextRoot, formattingState)
{
}

void InlineFormattingContext::layout() const
{
    if (!is<Container>(root()))
        return;

    LOG_WITH_STREAM(FormattingContextLayout, stream << "[Start] -> inline formatting context -> formatting root(" << &root() << ")");

    auto& inlineFormattingState = downcast<InlineFormattingState>(formattingState());
    InlineRunProvider inlineRunProvider(inlineFormattingState);

    collectInlineContent(inlineRunProvider);
    // Compute width/height for non-text content.
    for (auto& inlineRun : inlineRunProvider.runs()) {
        if (inlineRun.isText())
            continue;

        auto& layoutBox = inlineRun.inlineItem().layoutBox();
        if (layoutBox.establishesFormattingContext()) {
            layoutFormattingContextRoot(layoutBox);
            continue;
        }
        computeWidthAndHeightForReplacedInlineBox(layoutBox);
    }

    layoutInlineContent(inlineRunProvider);
    LOG_WITH_STREAM(FormattingContextLayout, stream << "[End] -> inline formatting context -> formatting root(" << &root() << ")");
}

static bool isTrimmableContent(const InlineLineBreaker::Run& run)
{
    return run.content.isWhitespace() && run.content.style().collapseWhiteSpace();
}

void InlineFormattingContext::initializeNewLine(Line& line) const
{
    auto& formattingRoot = downcast<Container>(root());
    auto& formattingRootDisplayBox = layoutState().displayBoxForLayoutBox(formattingRoot);

    auto lineLogicalLeft = formattingRootDisplayBox.contentBoxLeft();
    auto lineLogicalTop = line.isFirstLine() ? formattingRootDisplayBox.contentBoxTop() : line.logicalBottom();
    auto availableWidth = formattingRootDisplayBox.contentBoxWidth();

    // Check for intruding floats and adjust logical left/available width for this line accordingly.
    auto& floatingState = formattingState().floatingState();
    if (!floatingState.isEmpty()) {
        auto floatConstraints = floatingState.constraints(lineLogicalTop, formattingRoot);
        // Check if these constraints actually put limitation on the line.
        if (floatConstraints.left && *floatConstraints.left <= formattingRootDisplayBox.contentBoxLeft())
            floatConstraints.left = { };

        if (floatConstraints.right && *floatConstraints.right >= formattingRootDisplayBox.contentBoxRight())
            floatConstraints.right = { };

        if (floatConstraints.left && floatConstraints.right) {
            ASSERT(*floatConstraints.left < *floatConstraints.right);
            availableWidth = *floatConstraints.right - *floatConstraints.left;
            lineLogicalLeft = *floatConstraints.left;
        } else if (floatConstraints.left) {
            ASSERT(*floatConstraints.left > lineLogicalLeft);
            availableWidth -= (*floatConstraints.left - lineLogicalLeft);
            lineLogicalLeft = *floatConstraints.left;
        } else if (floatConstraints.right) {
            ASSERT(*floatConstraints.right > lineLogicalLeft);
            availableWidth = *floatConstraints.right - lineLogicalLeft;
        }
    }

    Display::Box::Rect logicalRect;
    logicalRect.setTop(lineLogicalTop);
    logicalRect.setLeft(lineLogicalLeft);
    logicalRect.setWidth(availableWidth);
    logicalRect.setHeight(formattingRoot.style().computedLineHeight());

    line.init(logicalRect);
}

void InlineFormattingContext::layoutInlineContent(const InlineRunProvider& inlineRunProvider) const
{
    auto& layoutState = this->layoutState();
    auto& inlineFormattingState = downcast<InlineFormattingState>(formattingState());
    auto floatingContext = FloatingContext { inlineFormattingState.floatingState() };

    Line line(inlineFormattingState, root());
    initializeNewLine(line);

    InlineLineBreaker lineBreaker(layoutState, inlineFormattingState.inlineContent(), inlineRunProvider.runs());
    while (auto run = lineBreaker.nextRun(line.contentLogicalRight(), line.availableWidth(), !line.hasContent())) {
        auto isFirstRun = run->position == InlineLineBreaker::Run::Position::LineBegin;
        auto isLastRun = run->position == InlineLineBreaker::Run::Position::LineEnd;
        auto generatesInlineRun = true;

        // Position float and adjust the runs on line.
        if (run->content.isFloat()) {
            auto& floatBox = run->content.inlineItem().layoutBox();
            computeFloatPosition(floatingContext, line, floatBox);
            inlineFormattingState.floatingState().append(floatBox);

            auto floatBoxWidth = layoutState.displayBoxForLayoutBox(floatBox).width();
            // Shrink availble space for current line and move existing inline runs.
            floatBox.isLeftFloatingPositioned() ? line.adjustLogicalLeft(floatBoxWidth) : line.adjustLogicalRight(floatBoxWidth);

            generatesInlineRun = false;
        }

        // 1. Initialize new line if needed.
        // 2. Append inline run unless it is skipped.
        // 3. Close current line if needed.
        if (isFirstRun) {
            // When the first run does not generate an actual inline run, the next run comes in first-run as well.
            // No need to spend time on closing/initializing.
            // Skip leading whitespace.
            if (!generatesInlineRun || isTrimmableContent(*run))
                continue;

            if (line.hasContent()) {
                // Previous run ended up being at the line end. Adjust the line accordingly.
                if (!line.isClosed())
                    line.close(Line::LastLine::No);
                initializeNewLine(line);
            }
         }

        if (generatesInlineRun)
             line.appendContent(*run);

        if (isLastRun)
            line.close(Line::LastLine::No);
    }

    line.close(Line::LastLine::Yes);
}

void InlineFormattingContext::computeWidthAndMargin(const Box& layoutBox) const
{
    auto& layoutState = this->layoutState();

    WidthAndMargin widthAndMargin;
    if (layoutBox.isFloatingPositioned())
        widthAndMargin = Geometry::floatingWidthAndMargin(layoutState, layoutBox);
    else if (layoutBox.isInlineBlockBox())
        widthAndMargin = Geometry::inlineBlockWidthAndMargin(layoutState, layoutBox);
    else if (layoutBox.replaced())
        widthAndMargin = Geometry::inlineReplacedWidthAndMargin(layoutState, layoutBox);
    else
        ASSERT_NOT_REACHED();

    auto& displayBox = layoutState.displayBoxForLayoutBox(layoutBox);
    displayBox.setContentBoxWidth(widthAndMargin.width);
    displayBox.setHorizontalMargin(widthAndMargin.margin);
    displayBox.setHorizontalNonComputedMargin(widthAndMargin.nonComputedMargin);
}

void InlineFormattingContext::computeHeightAndMargin(const Box& layoutBox) const
{
    auto& layoutState = this->layoutState();

    HeightAndMargin heightAndMargin;
    if (layoutBox.isFloatingPositioned())
        heightAndMargin = Geometry::floatingHeightAndMargin(layoutState, layoutBox);
    else if (layoutBox.isInlineBlockBox())
        heightAndMargin = Geometry::inlineBlockHeightAndMargin(layoutState, layoutBox);
    else if (layoutBox.replaced())
        heightAndMargin = Geometry::inlineReplacedHeightAndMargin(layoutState, layoutBox);
    else
        ASSERT_NOT_REACHED();

    auto& displayBox = layoutState.displayBoxForLayoutBox(layoutBox);
    displayBox.setContentBoxHeight(heightAndMargin.height);
    displayBox.setVerticalNonCollapsedMargin(heightAndMargin.margin);
    displayBox.setVerticalMargin(heightAndMargin.collapsedMargin.value_or(heightAndMargin.margin));
}

void InlineFormattingContext::layoutFormattingContextRoot(const Box& layoutBox) const
{
    ASSERT(layoutBox.isFloatingPositioned() || layoutBox.isInlineBlockBox());

    auto& layoutState = this->layoutState();
    auto& formattingState = layoutState.createFormattingStateForFormattingRootIfNeeded(layoutBox);
    computeBorderAndPadding(layoutBox);
    computeWidthAndMargin(layoutBox);
    // Swich over to the new formatting context (the one that the root creates).
    formattingState.formattingContext(layoutBox)->layout();
    // Come back and finalize the root's height and margin.
    computeHeightAndMargin(layoutBox);
}

void InlineFormattingContext::computeWidthAndHeightForReplacedInlineBox(const Box& layoutBox) const
{
    ASSERT(!layoutBox.isContainer());
    ASSERT(!layoutBox.establishesFormattingContext());
    ASSERT(layoutBox.replaced());

    computeBorderAndPadding(layoutBox);
    computeWidthAndMargin(layoutBox);
    computeHeightAndMargin(layoutBox);
}

void InlineFormattingContext::computeFloatPosition(const FloatingContext& floatingContext, Line& line, const Box& floatBox) const
{
    auto& layoutState = this->layoutState();
    ASSERT(layoutState.hasDisplayBox(floatBox));
    auto& displayBox = layoutState.displayBoxForLayoutBox(floatBox);

    // Set static position first.
    displayBox.setTopLeft({ line.contentLogicalRight(), line.logicalTop() });
    // Float it.
    displayBox.setTopLeft(floatingContext.positionForFloat(floatBox));
}

void InlineFormattingContext::computeStaticPosition(const Box&) const
{
}

void InlineFormattingContext::computeInFlowPositionedPosition(const Box&) const
{
}

void InlineFormattingContext::collectInlineContent(InlineRunProvider& inlineRunProvider) const
{
    if (!is<Container>(root()))
        return;

    auto& formattingRoot = downcast<Container>(root());
    auto* layoutBox = formattingRoot.firstInFlowOrFloatingChild();

    while (layoutBox) {
        ASSERT(layoutBox->isDescendantOf(formattingRoot));

        if (layoutBox->establishesFormattingContext()) {
            inlineRunProvider.append(*layoutBox);
            layoutBox = layoutBox->nextInFlowOrFloatingSibling();
            continue;
        }

        if (is<Container>(layoutBox)) {
            layoutBox = downcast<Container>(*layoutBox).firstInFlowOrFloatingChild();
            continue;
        }

        inlineRunProvider.append(*layoutBox);

        while (true) {
            if (auto* nextSibling = layoutBox->nextInFlowOrFloatingSibling()) {
                layoutBox = nextSibling;
                break;
            }

            layoutBox = layoutBox->parent();

            if (layoutBox == &formattingRoot)
                return;
        }
    }
}

FormattingContext::InstrinsicWidthConstraints InlineFormattingContext::instrinsicWidthConstraints() const
{
    auto& formattingStateForRoot = layoutState().formattingStateForBox(root());
    if (auto instrinsicWidthConstraints = formattingStateForRoot.instrinsicWidthConstraints(root()))
        return *instrinsicWidthConstraints;

    auto& inlineFormattingState = downcast<InlineFormattingState>(formattingState());
    InlineRunProvider inlineRunProvider(inlineFormattingState);
    collectInlineContent(inlineRunProvider);

    // Compute width for non-text content.
    for (auto& inlineRun : inlineRunProvider.runs()) {
        if (inlineRun.isText())
            continue;

        computeWidthAndMargin(inlineRun.inlineItem().layoutBox());
    }

    auto maximumLineWidth = [&](auto availableWidth) {
        LayoutUnit maxContentLogicalRight;
        InlineLineBreaker lineBreaker(layoutState(), inlineFormattingState.inlineContent(), inlineRunProvider.runs());
        LayoutUnit lineLogicalRight;
        while (auto run = lineBreaker.nextRun(lineLogicalRight, availableWidth, !lineLogicalRight)) {
            if (run->position == InlineLineBreaker::Run::Position::LineBegin)
                lineLogicalRight = 0;
            lineLogicalRight += run->width;

            maxContentLogicalRight = std::max(maxContentLogicalRight, lineLogicalRight);
        }
        return maxContentLogicalRight;
    };

    auto instrinsicWidthConstraints = FormattingContext::InstrinsicWidthConstraints { maximumLineWidth(0), maximumLineWidth(LayoutUnit::max()) };
    formattingStateForRoot.setInstrinsicWidthConstraints(root(), instrinsicWidthConstraints);
    return instrinsicWidthConstraints;
}

}
}

#endif
