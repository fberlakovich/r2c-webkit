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

#pragma once

#if ENABLE(LAYOUT_FORMATTING_CONTEXT)

#include "DisplayBox.h"
#include "FormattingContext.h"
#include "InlineLineBreaker.h"
#include <wtf/IsoMalloc.h>

namespace WebCore {
namespace Layout {

class InlineFormattingState;
class InlineRunProvider;

// This class implements the layout logic for inline formatting contexts.
// https://www.w3.org/TR/CSS22/visuren.html#inline-formatting
class InlineFormattingContext : public FormattingContext {
    WTF_MAKE_ISO_ALLOCATED(InlineFormattingContext);
public:
    InlineFormattingContext(const Box& formattingContextRoot, FormattingState&);

    void layout() const override;

private:
    class Line {
    public:
        Line(InlineFormattingState&, const Box& formattingRoot);

        void init(const Display::Box::Rect&);

        void appendContent(const InlineLineBreaker::Run&);

        void adjustLogicalLeft(LayoutUnit delta);
        void adjustLogicalRight(LayoutUnit delta);

        enum class LastLine { No, Yes };
        void close(LastLine = LastLine::No);

        bool hasContent() const { return m_firstRunIndex.has_value(); }
        bool isClosed() const { return m_closed; }
        bool isFirstLine() const { return m_isFirstLine; }
        LayoutUnit contentLogicalRight();
        LayoutUnit availableWidth() const { return m_availableWidth; }

        LayoutUnit logicalTop() const { return m_logicalRect.top(); }
        LayoutUnit logicalBottom() const { return m_logicalRect.bottom(); }

    private:
        void justifyRuns();
        void computeExpansionOpportunities(const InlineLineBreaker::Run&);

        struct TrailingTrimmableContent {
            LayoutUnit width;
            unsigned length;
        };
        std::optional<TrailingTrimmableContent> m_trailingTrimmableContent;
        bool m_lastRunIsWhitespace { false };
        bool m_lastRunCanExpand { false };

        InlineFormattingState& m_formattingState;
        const Box& m_formattingRoot;

        Display::Box::Rect m_logicalRect;
        LayoutUnit m_availableWidth;

        std::optional<unsigned> m_firstRunIndex;
        bool m_alignmentIsJustify { false };
        bool m_isFirstLine { true };
        bool m_closed { true };
    };

    // This class implements positioning and sizing for boxes participating in a block formatting context.
    class Geometry : public FormattingContext::Geometry {
    public:
        static HeightAndMargin inlineBlockHeightAndMargin(const LayoutState&, const Box&);
        static WidthAndMargin inlineBlockWidthAndMargin(const LayoutState&, const Box&);
    };

    void layoutInlineContent(const InlineRunProvider&) const;
    void initializeNewLine(Line&) const;

    void layoutFormattingContextRoot(const Box&) const;
    void computeWidthAndHeightForInlineBox(const Box&) const;
    void computeFloatPosition(const FloatingContext&, Line&, const Box&) const;
    void computeStaticPosition(const Box&) const override;
    void computeInFlowPositionedPosition(const Box&) const override;

    InstrinsicWidthConstraints instrinsicWidthConstraints(const Box&) const override;
};

}
}
#endif
