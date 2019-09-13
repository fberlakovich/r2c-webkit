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
#include "FormattingContext.h"

#if ENABLE(LAYOUT_FORMATTING_CONTEXT)

#include "DisplayBox.h"
#include "FormattingState.h"
#include "LayoutBox.h"
#include "LayoutContainer.h"
#include "LayoutDescendantIterator.h"
#include "LayoutState.h"
#include "Logging.h"
#include <wtf/IsoMallocInlines.h>
#include <wtf/text/TextStream.h>

namespace WebCore {
namespace Layout {

WTF_MAKE_ISO_ALLOCATED_IMPL(FormattingContext);

FormattingContext::FormattingContext(const Container& formattingContextRoot, FormattingState& formattingState)
    : m_root(makeWeakPtr(formattingContextRoot))
    , m_formattingState(formattingState)
{
#ifndef NDEBUG
    layoutState().registerFormattingContext(*this);
#endif
}

FormattingContext::~FormattingContext()
{
#ifndef NDEBUG
    layoutState().deregisterFormattingContext(*this);
#endif
}

LayoutState& FormattingContext::layoutState() const
{
    return m_formattingState.layoutState();
}

void FormattingContext::computeOutOfFlowHorizontalGeometry(const Box& layoutBox)
{
    auto containingBlockWidth = geometryForBox(*layoutBox.containingBlock()).paddingBoxWidth();

    auto compute = [&](Optional<LayoutUnit> usedWidth) {
        auto usedValues = UsedHorizontalValues { containingBlockWidth, usedWidth, { } };
        return geometry().outOfFlowHorizontalGeometry(layoutBox, usedValues);
    };

    auto horizontalGeometry = compute({ });
    if (auto maxWidth = geometry().computedValueIfNotAuto(layoutBox.style().logicalMaxWidth(), containingBlockWidth)) {
        auto maxHorizontalGeometry = compute(maxWidth);
        if (horizontalGeometry.widthAndMargin.width > maxHorizontalGeometry.widthAndMargin.width)
            horizontalGeometry = maxHorizontalGeometry;
    }

    if (auto minWidth = geometry().computedValueIfNotAuto(layoutBox.style().logicalMinWidth(), containingBlockWidth)) {
        auto minHorizontalGeometry = compute(minWidth);
        if (horizontalGeometry.widthAndMargin.width < minHorizontalGeometry.widthAndMargin.width)
            horizontalGeometry = minHorizontalGeometry;
    }

    auto& displayBox = formattingState().displayBox(layoutBox);
    displayBox.setLeft(horizontalGeometry.left + horizontalGeometry.widthAndMargin.usedMargin.start);
    displayBox.setContentBoxWidth(horizontalGeometry.widthAndMargin.width);
    displayBox.setHorizontalMargin(horizontalGeometry.widthAndMargin.usedMargin);
    displayBox.setHorizontalComputedMargin(horizontalGeometry.widthAndMargin.computedMargin);
}

void FormattingContext::computeOutOfFlowVerticalGeometry(const Box& layoutBox)
{
    auto compute = [&](UsedVerticalValues usedValues) {
        return geometry().outOfFlowVerticalGeometry(layoutBox, usedValues);
    };

    auto verticalGeometry = compute({ });
    if (auto maxHeight = geometry().computedMaxHeight(layoutBox)) {
        auto maxVerticalGeometry = compute({ *maxHeight });
        if (verticalGeometry.heightAndMargin.height > maxVerticalGeometry.heightAndMargin.height)
            verticalGeometry = maxVerticalGeometry;
    }

    if (auto minHeight = geometry().computedMinHeight(layoutBox)) {
        auto minVerticalGeometry = compute({ *minHeight });
        if (verticalGeometry.heightAndMargin.height < minVerticalGeometry.heightAndMargin.height)
            verticalGeometry = minVerticalGeometry;
    }

    auto& displayBox = formattingState().displayBox(layoutBox);
    auto nonCollapsedVerticalMargin = verticalGeometry.heightAndMargin.nonCollapsedMargin;
    displayBox.setTop(verticalGeometry.top + nonCollapsedVerticalMargin.before);
    displayBox.setContentBoxHeight(verticalGeometry.heightAndMargin.height);
    // Margins of absolutely positioned boxes do not collapse
    displayBox.setVerticalMargin({ nonCollapsedVerticalMargin, { } });
}

void FormattingContext::computeBorderAndPadding(const Box& layoutBox, Optional<UsedHorizontalValues> usedValues)
{
    if (!usedValues)
        usedValues = UsedHorizontalValues { geometryForBox(*layoutBox.containingBlock()).contentBoxWidth() };
    auto& displayBox = formattingState().displayBox(layoutBox);
    displayBox.setBorder(geometry().computedBorder(layoutBox));
    displayBox.setPadding(geometry().computedPadding(layoutBox, *usedValues));
}

void FormattingContext::layoutOutOfFlowContent()
{
    LOG_WITH_STREAM(FormattingContextLayout, stream << "Start: layout out-of-flow content -> context: " << &layoutState() << " root: " << &root());

    for (auto& outOfFlowBox : formattingState().outOfFlowBoxes()) {
        ASSERT(outOfFlowBox->establishesFormattingContext());

        computeBorderAndPadding(*outOfFlowBox);
        computeOutOfFlowHorizontalGeometry(*outOfFlowBox);
        if (is<Container>(*outOfFlowBox)) {
            auto formattingContext = layoutState().createFormattingContext(downcast<Container>(*outOfFlowBox));
            formattingContext->layoutInFlowContent();
            computeOutOfFlowVerticalGeometry(*outOfFlowBox);
            formattingContext->layoutOutOfFlowContent();            
        } else
            computeOutOfFlowVerticalGeometry(*outOfFlowBox);
    }
    LOG_WITH_STREAM(FormattingContextLayout, stream << "End: layout out-of-flow content -> context: " << &layoutState() << " root: " << &root());
}

static LayoutUnit mapHorizontalPositionToAncestor(const FormattingContext& formattingContext, LayoutUnit horizontalPosition, const Container& containingBlock, const Container& ancestor)
{
    // "horizontalPosition" is in the coordinate system of the "containingBlock". -> map from containingBlock to ancestor.
    if (&containingBlock == &ancestor)
        return horizontalPosition;
    ASSERT(containingBlock.isContainingBlockDescendantOf(ancestor));
    for (auto* container = &containingBlock; container && container != &ancestor; container = container->containingBlock())
        horizontalPosition += formattingContext.geometryForBox(*container).left();
    return horizontalPosition;
}

// FIXME: turn these into templates.
LayoutUnit FormattingContext::mapTopToFormattingContextRoot(const Box& layoutBox) const
{
    ASSERT(layoutBox.containingBlock());
    auto& formattingContextRoot = root();
    ASSERT(layoutBox.isContainingBlockDescendantOf(formattingContextRoot));
    auto top = geometryForBox(layoutBox).top();
    for (auto* container = layoutBox.containingBlock(); container && container != &formattingContextRoot; container = container->containingBlock())
        top += geometryForBox(*container).top();
    return top;
}

LayoutUnit FormattingContext::mapLeftToFormattingContextRoot(const Box& layoutBox) const
{
    ASSERT(layoutBox.containingBlock());
    return mapHorizontalPositionToAncestor(*this, geometryForBox(layoutBox).left(), *layoutBox.containingBlock(), root());
}

LayoutUnit FormattingContext::mapRightToFormattingContextRoot(const Box& layoutBox) const
{
    ASSERT(layoutBox.containingBlock());
    return mapHorizontalPositionToAncestor(*this, geometryForBox(layoutBox).right(), *layoutBox.containingBlock(), root());
}

const Display::Box& FormattingContext::geometryForBox(const Box& layoutBox, Optional<EscapeType> escapeType) const
{
    UNUSED_PARAM(escapeType);
#ifndef NDEBUG
    auto isOkToAccessDisplayBox = [&] {
        // 1. Highly common case of accessing the formatting root's display box itself. This is formatting context escaping in the strict sense, since
        // the formatting context root box lives in the parent formatting context.
        // This happens e.g. when a block level box box needs to stretch horizontally and checks its containing block for horizontal space (this should probably be limited to reading horizontal constraint values).
        if (&layoutBox == &root())
            return true;

        // 2. Special case when accessing the ICB's display box
        if (layoutBox.isInitialContainingBlock()) {
            // There has to be a valid reason to access the ICB.
            if (!escapeType)
                return false;
            return *escapeType == EscapeType::AccessParentFormattingContext || *escapeType == EscapeType::AccessAncestorFormattingContext;
        }

        // 3. Most common case of accessing box/containing block display box within the same formatting context tree.
        if (&layoutBox.formattingContextRoot() == &root())
            return true;

        if (!escapeType)
            return false;

        // 4. Accessing child formatting context subtree is relatively rare. It happens when e.g a shrink to fit (out-of-flow block level) box checks the content width.
        // Checking the content width means to get display boxes from the established formatting context (we try to access display boxes in a child formatting context)
        if (*escapeType == EscapeType::AccessChildFormattingContext && &layoutBox.formattingContextRoot().formattingContextRoot() == &root())
            return true;

        // 5. Float box top/left values are mapped relative to the FloatState's root. Inline formatting contexts(A) inherit floats from parent
        // block formatting contexts(B). Floats in these inline formatting contexts(A) need to be mapped to the parent, block formatting context(B).
        if (*escapeType == EscapeType::AccessParentFormattingContext && &layoutBox.formattingContextRoot() == &root().formattingContextRoot())
            return true;

        // 6. Finding the first containing block with fixed height quirk. See Quirks::heightValueOfNearestContainingBlockWithFixedHeight
        if (*escapeType == EscapeType::AccessAncestorFormattingContext) {
            auto& targetFormattingRoot = layoutBox.formattingContextRoot();
            auto* ancestorFormattingContextRoot = &root().formattingContextRoot();
            while (true) {
                if (&targetFormattingRoot == ancestorFormattingContextRoot)
                    return true;
                if (ancestorFormattingContextRoot->isInitialContainingBlock())
                    return false;
                ancestorFormattingContextRoot = &ancestorFormattingContextRoot->formattingContextRoot();
            }

        }
        return false;
    };
#endif
    ASSERT(isOkToAccessDisplayBox());
    ASSERT(layoutState().hasDisplayBox(layoutBox));
    return layoutState().displayBoxForLayoutBox(layoutBox);
}

#ifndef NDEBUG
void FormattingContext::validateGeometryConstraintsAfterLayout() const
{
    auto& formattingContextRoot = root();
    // FIXME: add a descendantsOfType<> flavor that stops at nested formatting contexts
    for (auto& layoutBox : descendantsOfType<Box>(formattingContextRoot)) {
        if (&layoutBox.formattingContextRoot() != &formattingContextRoot)
            continue;
        auto& containingBlockGeometry = geometryForBox(*layoutBox.containingBlock());
        auto& boxGeometry = geometryForBox(layoutBox);

        // 10.3.3 Block-level, non-replaced elements in normal flow
        // 10.3.7 Absolutely positioned, non-replaced elements
        if ((layoutBox.isBlockLevelBox() || layoutBox.isOutOfFlowPositioned()) && !layoutBox.replaced()) {
            // margin-left + border-left-width + padding-left + width + padding-right + border-right-width + margin-right = width of containing block
            auto containingBlockWidth = containingBlockGeometry.contentBoxWidth();
            ASSERT(boxGeometry.horizontalMarginBorderAndPadding() + boxGeometry.contentBoxWidth() == containingBlockWidth);
        }

        // 10.6.4 Absolutely positioned, non-replaced elements
        if (layoutBox.isOutOfFlowPositioned() && !layoutBox.replaced()) {
            // top + margin-top + border-top-width + padding-top + height + padding-bottom + border-bottom-width + margin-bottom + bottom = height of containing block
            auto containingBlockHeight = containingBlockGeometry.contentBoxHeight();
            ASSERT(boxGeometry.top() + boxGeometry.marginBefore() + boxGeometry.borderTop() + boxGeometry.paddingTop().valueOr(0) + boxGeometry.contentBoxHeight()
                + boxGeometry.paddingBottom().valueOr(0) + boxGeometry.borderBottom() + boxGeometry.marginAfter() == containingBlockHeight);
        }
    }
}
#endif

}
}
#endif
