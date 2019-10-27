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
#include "LayoutContext.h"

#if ENABLE(LAYOUT_FORMATTING_CONTEXT)

#include "BlockFormattingContext.h"
#include "BlockFormattingState.h"
#include "BlockInvalidation.h"
#include "DisplayBox.h"
#include "DisplayPainter.h"
#include "InlineFormattingContext.h"
#include "InlineFormattingState.h"
#include "InlineInvalidation.h"
#include "Invalidation.h"
#include "LayoutBox.h"
#include "LayoutContainer.h"
#include "LayoutPhase.h"
#include "LayoutTreeBuilder.h"
#include "RenderView.h"
#include "TableFormattingContext.h"
#include "TableFormattingState.h"
#include <wtf/IsoMallocInlines.h>

namespace WebCore {
namespace Layout {

WTF_MAKE_ISO_ALLOCATED_IMPL(LayoutContext);

LayoutContext::LayoutContext(LayoutState& layoutState)
    : m_layoutState(layoutState)
{
}

void LayoutContext::layout()
{
    PhaseScope scope(Phase::Type::Layout);

    ASSERT(!m_formattingContextRootListForLayout.computesEmpty());
    for (auto& layoutRoot : m_formattingContextRootListForLayout)
        layoutFormattingContextSubtree(layoutRoot);
    m_formattingContextRootListForLayout.clear();
}

void LayoutContext::layoutFormattingContextSubtree(const Container& layoutRoot)
{
    RELEASE_ASSERT(layoutRoot.establishesFormattingContext());
    auto formattingContext = createFormattingContext(layoutRoot, layoutState());
    formattingContext->layoutInFlowContent();
    formattingContext->layoutOutOfFlowContent();
}

void LayoutContext::styleChanged(const Box& layoutBox, StyleDiff styleDiff)
{
    PhaseScope scope(Phase::Type::Invalidation);

    auto& formattingState = layoutState().formattingStateForBox(layoutBox);
    const Container* invalidationRoot = nullptr;
    if (is<BlockFormattingState>(formattingState))
        invalidationRoot = BlockInvalidation::invalidate(layoutBox, styleDiff, *this, downcast<BlockFormattingState>(formattingState)).root;
    else if (is<InlineFormattingState>(formattingState))
        invalidationRoot = InlineInvalidation::invalidate(layoutBox, styleDiff, *this, downcast<InlineFormattingState>(formattingState)).root;
    else
        ASSERT_NOT_IMPLEMENTED_YET();
    ASSERT(invalidationRoot);
    m_formattingContextRootListForLayout.add(invalidationRoot);
}

void LayoutContext::markNeedsUpdate(const Box& layoutBox, OptionSet<UpdateType>)
{
    // FIXME: This should trigger proper invalidation instead of just adding the formatting context root to the dirty list. 
    m_formattingContextRootListForLayout.add(&(layoutBox.isInitialContainingBlock() ? downcast<Container>(layoutBox) : layoutBox.formattingContextRoot()));
}

std::unique_ptr<FormattingContext> LayoutContext::createFormattingContext(const Container& formattingContextRoot, LayoutState& layoutState)
{
    ASSERT(formattingContextRoot.establishesFormattingContext());
    if (formattingContextRoot.establishesInlineFormattingContext()) {
        auto& inlineFormattingState = downcast<InlineFormattingState>(layoutState.createFormattingStateForFormattingRootIfNeeded(formattingContextRoot));
        return makeUnique<InlineFormattingContext>(formattingContextRoot, inlineFormattingState);
    }

    if (formattingContextRoot.establishesBlockFormattingContext()) {
        ASSERT(formattingContextRoot.establishesBlockFormattingContextOnly());
        auto& blockFormattingState = downcast<BlockFormattingState>(layoutState.createFormattingStateForFormattingRootIfNeeded(formattingContextRoot));
        return makeUnique<BlockFormattingContext>(formattingContextRoot, blockFormattingState);
    }

    if (formattingContextRoot.establishesTableFormattingContext()) {
        auto& tableFormattingState = downcast<TableFormattingState>(layoutState.createFormattingStateForFormattingRootIfNeeded(formattingContextRoot));
        return makeUnique<TableFormattingContext>(formattingContextRoot, tableFormattingState);
    }

    CRASH();
}

static void initializeLayoutState(LayoutState& layoutState, const RenderView& renderView)
{
    auto quirksMode = [&] {
        auto& document = renderView.document();
        if (document.inLimitedQuirksMode())
            return LayoutState::QuirksMode::Limited;
        if (document.inQuirksMode())
            return LayoutState::QuirksMode::Yes;
        return LayoutState::QuirksMode::No;
    };

    layoutState.setQuirksMode(quirksMode());

    auto& layoutRoot = layoutState.root();
    layoutState.createFormattingStateForFormattingRootIfNeeded(layoutRoot);
    // Not efficient, but this is temporary anyway.
    // Collect the out-of-flow descendants at the formatting root level (as opposed to at the containing block level, though they might be the same).
    for (auto& descendant : descendantsOfType<Box>(layoutRoot)) {
        if (!descendant.isOutOfFlowPositioned())
            continue;
        auto& formattingState = layoutState.createFormattingStateForFormattingRootIfNeeded(descendant.formattingContextRoot());
        formattingState.addOutOfFlowBox(descendant);
    }
}

void LayoutContext::runLayout(LayoutState& layoutState)
{
    auto& layoutRoot = layoutState.root();
    auto& displayBox = layoutState.displayBoxForLayoutBox(layoutRoot);
    displayBox.setHorizontalMargin({ });
    displayBox.setHorizontalComputedMargin({ });
    displayBox.setVerticalMargin({ });
    displayBox.setBorder({ });
    displayBox.setPadding({ });
    displayBox.setTopLeft({ });
    displayBox.setContentBoxHeight(LayoutUnit(layoutRoot.style().logicalHeight().value()));
    displayBox.setContentBoxWidth(LayoutUnit(layoutRoot.style().logicalWidth().value()));

    auto layoutContext = LayoutContext(layoutState);
    layoutContext.markNeedsUpdate(layoutRoot);
    layoutContext.layout();
}

std::unique_ptr<LayoutState> LayoutContext::runLayoutAndVerify(const RenderView& renderView)
{
    auto layoutState = makeUnique<LayoutState>(TreeBuilder::createLayoutTree(renderView));
    initializeLayoutState(*layoutState, renderView);
    runLayout(*layoutState);
    LayoutContext::verifyAndOutputMismatchingLayoutTree(*layoutState, renderView);
    return layoutState;
}

void LayoutContext::paint(const LayoutState& layoutState, GraphicsContext& context, const IntRect& dirtyRect)
{
    Display::Painter::paint(layoutState, context, dirtyRect);
}

}
}

#endif
