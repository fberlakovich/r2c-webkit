/*
 * Copyright (C) 2018 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

var Layout = { }

Layout.Box = class Box {
    constructor(node, id) {
        this.m_id = id;
        this.m_rendererName = null;
        this.m_node = node;
        this.m_parent = null;
        this.m_nextSibling = null;
        this.m_previousSibling = null;
        this.m_isAnonymous = false;
        this.m_establishedFormattingContext = null;
        this.m_displayBox = null;
    }

    id() {
        return this.m_id;
    }

    setRendererName(name) {
        this.m_rendererName = name;
    }

    name() {
        return this.m_rendererName;
    }

    node() {
        return this.m_node;
    }

    parent() {
        return this.m_parent;
    }

    nextSibling() {
        return this.m_nextSibling;
    }

    nextInFlowSibling() {
        let nextInFlowSibling = this.nextSibling();
        while (nextInFlowSibling) {
            if (nextInFlowSibling.isInFlow())
                return nextInFlowSibling;
            nextInFlowSibling = nextInFlowSibling.nextSibling();
        }
        return null;
    }

    nextInFlowOrFloatSibling() {
        let nextInFlowSibling = this.nextSibling();
        while (nextInFlowSibling) {
            if (nextInFlowSibling.isInFlow() || nextInFlowSibling.isFloatingPositioned())
                return nextInFlowSibling;
            nextInFlowSibling = nextInFlowSibling.nextSibling();
        }
        return null;
    }

    previousSibling() {
        return this.m_previousSibling;
    }

    previousInFlowSibling() {
        let previousInFlowSibling = this.previousSibling();
        while (previousInFlowSibling) {
            if (previousInFlowSibling.isInFlow())
                return previousInFlowSibling;
            previousInFlowSibling = previousInFlowSibling.previousSibling();
        }
        return null;
    }

    setParent(parent) {
        this.m_parent = parent;
    }

    setNextSibling(nextSibling) {
        this.m_nextSibling = nextSibling;
    }
    
    setPreviousSibling(previousSibling) {
        this.m_previousSibling = previousSibling;
    }

    setDisplayBox(displayBox) {
        ASSERT(!this.m_displayBox);
        this.m_displayBox = displayBox;
    }

    displayBox() {
        ASSERT(this.m_displayBox);
        return this.m_displayBox;
    }

    rect() {
        return this.displayBox().rect();
    }

    topLeft() {
        return this.rect().topLeft();
    }

    bottomRight() {
        return this.rect().bottomRight();
    }

    setTopLeft(topLeft) {
        this.displayBox().setTopLeft(topLeft);
    }

    setSize(size) {
        this.displayBox().setSize(size);
    }

    setWidth(width) {
        this.displayBox().setWidth(width);
    }

    setHeight(height) {
        this.displayBox().setHeight(height);
    }

    isContainer() {
        return false;
    }

    isBlockLevelBox() {
        return Utils.isBlockLevelElement(this.node());
    }

    isBlockContainerBox() {
        return Utils.isBlockContainerElement(this.node());
    }

    isInlineLevelBox() {
        return Utils.isInlineLevelElement(this.node());
    }

    setIsAnonymous() {
        this.m_isAnonymous = true;
    }

    isAnonymous() {
        return this.m_isAnonymous;
    }

    establishesFormattingContext() {
        if (this.isAnonymous())
            return false;
        return this.establishesBlockFormattingContext() || this.establishesInlineFormattingContext();
    }

    establishedFormattingContext() {
        if (this.establishesFormattingContext() && !this.m_establishedFormattingContext)
            this.m_establishedFormattingContext = this.establishesBlockFormattingContext() ? new BlockFormattingContext(this) : new InlineFormattingContext(this);
        return this.m_establishedFormattingContext;
    }

    establishesBlockFormattingContext() {
        // 9.4.1 Block formatting contexts
        // Floats, absolutely positioned elements, block containers (such as inline-blocks, table-cells, and table-captions)
        // that are not block boxes, and block boxes with 'overflow' other than 'visible' (except when that value has been propagated to the viewport)
        // establish new block formatting contexts for their contents.
        return this.isFloatingPositioned() || this.isAbsolutelyPositioned() || (this.isBlockContainerBox() && !this.isBlockLevelBox())
            || (this.isBlockLevelBox() && !Utils.isOverflowVisible(this));
    }

    establishesInlineFormattingContext() {
        return false;
    }

    isPositioned() {
        return this.isOutOfFlowPositioned() || this.isRelativelyPositioned();
    }

    isRelativelyPositioned() {
        return Utils.isRelativelyPositioned(this);
    }

    isAbsolutelyPositioned() {
        return Utils.isAbsolutelyPositioned(this) || this.isFixedPositioned();
    }

    isFixedPositioned() {
        return Utils.isFixedPositioned(this);
    }

    isInFlow() {
        if (this.isAnonymous())
            return true;
        return !this.isFloatingOrOutOfFlowPositioned();
    }

    isOutOfFlowPositioned() {
        return this.isAbsolutelyPositioned() || this.isFixedPositioned();
    }

    isInFlowPositioned() {
        return this.isPositioned() && !this.isOutOfFlowPositioned();
    }

    isFloatingPositioned() {
        return Utils.isFloatingPositioned(this);
    }

    isFloatingOrOutOfFlowPositioned() {
        return this.isFloatingPositioned() || this.isOutOfFlowPositioned();
    }

    isRootBox() {
        // FIXME: This should just be a simple instanceof check, but we are in the mainframe while the test document is in an iframe
        // Let's just return root for both the RenderView and the <html> element.
        return !this.parent() || !this.parent().parent();
    }

    containingBlock() {
        if (!this.parent())
            return null;
        if (!this.isPositioned() || this.isInFlowPositioned())
            return this.parent();
        if (this.isAbsolutelyPositioned() && !this.isFixedPositioned()) {
            let ascendant = this.parent();
            while (ascendant.parent() && !ascendant.isPositioned())
                ascendant = ascendant.parent();
            return ascendant;
        }
        if (this.isFixedPositioned()) {
            let ascendant = this.parent();
            while (ascendant.parent())
                ascendant = ascendant.parent();
            return ascendant;
        }
        ASSERT_NOT_REACHED();
        return null;
    }

    isDescendantOf(container) {
        ASSERT(container);
        let ascendant = this.parent();
        while (ascendant && ascendant != container)
            ascendant = ascendant.parent();
        return !!ascendant;
    }

    borderBox() {
        return this.displayBox().borderBox();
    }

    paddingBox() {
        return this.displayBox().paddingBox();
    }

    contentBox() {
        return this.displayBox().contentBox();
    }
}
