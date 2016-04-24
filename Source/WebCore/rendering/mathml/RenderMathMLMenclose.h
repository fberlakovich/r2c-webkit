/*
 * Copyright (C) 2014 Gurpreet Kaur (k.gurpreet@samsung.com). All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RenderMathMLMenclose_h
#define RenderMathMLMenclose_h

#if ENABLE(MATHML)
#include "RenderMathMLRow.h"

namespace WebCore {
    
class RenderMathMLMenclose final: public RenderMathMLRow {
public:
    RenderMathMLMenclose(Element&, std::unique_ptr<RenderStyle>);

private:
    bool isRenderMathMLMenclose() const final { return true; }
    const char* renderName() const final { return "RenderMathMLMenclose"; }
    void paint(PaintInfo&, const LayoutPoint&) final;
    void updateLogicalHeight() override;
    void addChild(RenderObject* newChild, RenderObject* beforeChild = nullptr) override;
    void computePreferredLogicalWidths() final;
    bool checkNotationalValuesValidity(const Vector<String>&) const;
};
    
}

SPECIALIZE_TYPE_TRAITS_RENDER_OBJECT(RenderMathMLMenclose, isRenderMathMLMenclose())

#endif // ENABLE(MATHML)
#endif // RenderMathMLMenclose_h
