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
#include "HighlightRangeGroup.h"

#include "IDLTypes.h"
#include "JSDOMSetLike.h"
#include "JSStaticRange.h"
#include "NodeTraversal.h"
#include "PropertySetCSSStyleDeclaration.h"
#include "RenderObject.h"
#include "StaticRange.h"
#include "StyleProperties.h"
#include <wtf/Ref.h>

namespace WebCore {

HighlightRangeGroup::HighlightRangeGroup(Ref<StaticRange>&& range)
{
    auto myRange = WTFMove(range);
    addToSetLike(myRange.get());
}

Ref<HighlightRangeGroup> HighlightRangeGroup::create(StaticRange& range)
{
    return adoptRef(*new HighlightRangeGroup(range));
}

void HighlightRangeGroup::initializeSetLike(DOMSetAdapter& set)
{
    for (auto& rangeData : m_rangesData)
        set.add<IDLInterface<StaticRange>>(rangeData->range);
}

static void repaintRange(const StaticRange& range)
{
    auto* startNode = &range.startContainer();
    auto* endNode = &range.endContainer();
    auto ordering = documentOrder(*startNode, *endNode);
    if (is_eq(ordering)) {
        if (auto renderer = startNode->renderer())
            renderer->repaint();
        return;
    }
    if (is_gt(ordering)) {
        startNode = &range.endContainer();
        endNode = &range.startContainer();
    }

    auto node = startNode;
    while (node != endNode) {
        if (auto renderer = node->renderer())
            renderer->repaint();
        node = NodeTraversal::next(*node);
    }
}

bool HighlightRangeGroup::removeFromSetLike(const StaticRange& range)
{
    return m_rangesData.removeFirstMatching([&range](const Ref<HighlightRangeData>& current) {
        repaintRange(range);
        return current.get().range.get() == range;
    });
}

void HighlightRangeGroup::clearFromSetLike()
{
    for (auto& data : m_rangesData)
        repaintRange(data->range);

    m_rangesData.clear();
}

bool HighlightRangeGroup::addToSetLike(StaticRange& range)
{
    if (notFound != m_rangesData.findMatching([&range](const Ref<HighlightRangeData>& current) { return current.get().range.get() == range; }))
        return false;
    repaintRange(range);
    m_rangesData.append(HighlightRangeData::create(range));
    
    return true;
}

}

