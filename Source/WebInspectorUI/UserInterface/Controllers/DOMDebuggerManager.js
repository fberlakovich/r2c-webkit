/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

WebInspector.DOMDebuggerManager = class DOMDebuggerManager extends WebInspector.Object
{
    constructor()
    {
        super();

        this._domBreakpointsSetting = new WebInspector.Setting("dom-breakpoints", []);
        this._domBreakpointURLMap = new Map;
        this._domBreakpointFrameIdentifierMap = new Map;

        WebInspector.DOMBreakpoint.addEventListener(WebInspector.DOMBreakpoint.Event.DisabledStateDidChange, this._domBreakpointDisabledStateDidChange, this);

        WebInspector.domTreeManager.addEventListener(WebInspector.DOMTreeManager.Event.NodeRemoved, this._nodeRemoved, this);
        WebInspector.domTreeManager.addEventListener(WebInspector.DOMTreeManager.Event.NodeInserted, this._nodeInserted, this);

        WebInspector.frameResourceManager.addEventListener(WebInspector.FrameResourceManager.Event.MainFrameDidChange, this._mainFrameDidChange, this);

        WebInspector.Frame.addEventListener(WebInspector.Frame.Event.ChildFrameWasRemoved, this._childFrameWasRemoved, this);
        WebInspector.Frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainResourceDidChange, this);

        if (this.supported) {
            this._restoringBreakpoints = true;

            for (let cookie of this._domBreakpointsSetting.value) {
                let breakpoint = new WebInspector.DOMBreakpoint(cookie, cookie.type, cookie.disabled);
                this.addDOMBreakpoint(breakpoint);
            }

            this._restoringBreakpoints = false;
            this._speculativelyResolveBreakpoints();
        }
    }

    // Public

    get supported()
    {
        return !!window.DOMDebuggerAgent;
    }

    get domBreakpoints()
    {
        let mainFrame = WebInspector.frameResourceManager.mainFrame;
        if (!mainFrame)
            return [];

        let resolvedBreakpoints = [];
        let frames = [mainFrame];
        while (frames.length) {
            let frame = frames.shift();
            let domBreakpointNodeIdentifierMap = this._domBreakpointFrameIdentifierMap.get(frame.id);
            if (domBreakpointNodeIdentifierMap) {
                for (let breakpoints of domBreakpointNodeIdentifierMap.values())
                    resolvedBreakpoints = resolvedBreakpoints.concat(breakpoints);
            }

            frames = frames.concat(frame.childFrameCollection.toArray());
        }

        return resolvedBreakpoints;
    }

    domBreakpointsForNode(node)
    {
        console.assert(node instanceof WebInspector.DOMNode);

        if (!node)
            return [];

        let domBreakpointNodeIdentifierMap = this._domBreakpointFrameIdentifierMap.get(node.frameIdentifier);
        if (!domBreakpointNodeIdentifierMap)
            return [];

        let breakpoints = domBreakpointNodeIdentifierMap.get(node.id);
        return breakpoints ? breakpoints.slice() : [];
    }

    addDOMBreakpoint(breakpoint)
    {
        console.assert(breakpoint instanceof WebInspector.DOMBreakpoint);
        if (!breakpoint || !breakpoint.url)
            return;

        let breakpoints = this._domBreakpointURLMap.get(breakpoint.url);
        if (!breakpoints) {
            breakpoints = [breakpoint];
            this._domBreakpointURLMap.set(breakpoint.url, breakpoints);
        } else
            breakpoints.push(breakpoint);

        if (breakpoint.domNodeIdentifier)
            this._resolveDOMBreakpoint(breakpoint, breakpoint.domNodeIdentifier);

        this.dispatchEventToListeners(WebInspector.DOMDebuggerManager.Event.DOMBreakpointAdded, {breakpoint});

        if (!this._restoringBreakpoints)
            this._saveBreakpoints();
    }

    removeDOMBreakpoint(breakpoint)
    {
        console.assert(breakpoint instanceof WebInspector.DOMBreakpoint);
        if (!breakpoint)
            return;

        let nodeIdentifier = breakpoint.domNodeIdentifier;
        console.assert(nodeIdentifier, "Cannot remove unresolved DOM breakpoint.");
        if (!nodeIdentifier)
            return;

        this._detachDOMBreakpoint(breakpoint);

        let urlBreakpoints = this._domBreakpointURLMap.get(breakpoint.url);
        urlBreakpoints.remove(breakpoint, true);

        if (!breakpoint.disabled)
            DOMDebuggerAgent.removeDOMBreakpoint(nodeIdentifier, breakpoint.type);

        if (!urlBreakpoints.length)
            this._domBreakpointURLMap.delete(breakpoint.url);

        this.dispatchEventToListeners(WebInspector.DOMDebuggerManager.Event.DOMBreakpointRemoved, {breakpoint});

        breakpoint.domNodeIdentifier = null;

        this._saveBreakpoints();
    }

    // Private

    _detachDOMBreakpoint(breakpoint)
    {
        let nodeIdentifier = breakpoint.domNodeIdentifier;
        let node = WebInspector.domTreeManager.nodeForId(nodeIdentifier);
        console.assert(node, "Missing DOM node for breakpoint.", breakpoint);
        if (!node)
            return;

        let frameIdentifier = node.frameIdentifier;
        let domBreakpointNodeIdentifierMap = this._domBreakpointFrameIdentifierMap.get(frameIdentifier);
        console.assert(domBreakpointNodeIdentifierMap, "Missing DOM breakpoints for node parent frame.", node);
        if (!domBreakpointNodeIdentifierMap)
            return;

        let breakpoints = domBreakpointNodeIdentifierMap.get(nodeIdentifier);
        console.assert(breakpoints, "Missing DOM breakpoints for node.", node);
        if (!breakpoints)
            return;

        breakpoints.remove(breakpoint, true);

        if (breakpoints.length)
            return;

        domBreakpointNodeIdentifierMap.delete(nodeIdentifier);

        if (!domBreakpointNodeIdentifierMap.size)
            this._domBreakpointFrameIdentifierMap.delete(frameIdentifier)
    }

    _detachBreakpointsForFrame(frame)
    {
        let domBreakpointNodeIdentifierMap = this._domBreakpointFrameIdentifierMap.get(frame.id);
        if (!domBreakpointNodeIdentifierMap)
            return;

        this._domBreakpointFrameIdentifierMap.delete(frame.id);

        for (let breakpoints of domBreakpointNodeIdentifierMap.values()) {
            for (let breakpoint of breakpoints)
                breakpoint.domNodeIdentifier = null;
        }
    }

    _speculativelyResolveBreakpoints()
    {
        let mainFrame = WebInspector.frameResourceManager.mainFrame;
        if (!mainFrame)
            return;

        let breakpoints = this._domBreakpointURLMap.get(mainFrame.url);
        if (!breakpoints)
            return;

        for (let breakpoint of breakpoints) {
            if (breakpoint.domNodeIdentifier)
                continue;

            WebInspector.domTreeManager.pushNodeByPathToFrontend(breakpoint.path, (nodeIdentifier) => {
                if (!nodeIdentifier)
                    return;

                this._resolveDOMBreakpoint(breakpoint, nodeIdentifier);
            });
        }
    }

    _resolveDOMBreakpoint(breakpoint, nodeIdentifier)
    {
        let node = WebInspector.domTreeManager.nodeForId(nodeIdentifier);
        console.assert(node, "Missing DOM node for nodeIdentifier.", nodeIdentifier);
        if (!node)
            return;

        let frameIdentifier = node.frameIdentifier;
        let domBreakpointNodeIdentifierMap = this._domBreakpointFrameIdentifierMap.get(frameIdentifier);
        if (!domBreakpointNodeIdentifierMap) {
            domBreakpointNodeIdentifierMap = new Map;
            this._domBreakpointFrameIdentifierMap.set(frameIdentifier, domBreakpointNodeIdentifierMap);
        }

        let breakpoints = domBreakpointNodeIdentifierMap.get(nodeIdentifier);
        if (breakpoints)
            breakpoints.push(breakpoint);
        else
            domBreakpointNodeIdentifierMap.set(nodeIdentifier, [breakpoint]);

        breakpoint.domNodeIdentifier = nodeIdentifier;

        this._updateDOMBreakpoint(breakpoint);
    }

    _updateDOMBreakpoint(breakpoint)
    {
        let nodeIdentifier = breakpoint.domNodeIdentifier;
        if (!nodeIdentifier)
            return;

        function breakpointUpdated(error)
        {
            if (error)
                console.error(error);
        }

        if (breakpoint.disabled)
            DOMDebuggerAgent.removeDOMBreakpoint(nodeIdentifier, breakpoint.type, breakpointUpdated);
        else
            DOMDebuggerAgent.setDOMBreakpoint(nodeIdentifier, breakpoint.type, breakpointUpdated);
    }

    _saveBreakpoints()
    {
        if (this._restoringBreakpoints)
            return;

        let breakpointsToSave = [];
        for (let breakpoints of this._domBreakpointURLMap.values())
            breakpointsToSave = breakpointsToSave.concat(breakpoints);

        this._domBreakpointsSetting.value = breakpointsToSave.map((breakpoint) => breakpoint.serializableInfo);
    }

    _domBreakpointDisabledStateDidChange(event)
    {
        let breakpoint = event.target;
        this._updateDOMBreakpoint(breakpoint);
        this._saveBreakpoints();
    }

    _childFrameWasRemoved(event)
    {
        let frame = event.data.childFrame;
        this._detachBreakpointsForFrame(frame);
    }

    _mainFrameDidChange()
    {
        this._speculativelyResolveBreakpoints();
    }

    _mainResourceDidChange(event)
    {
        let frame = event.target;
        if (frame.isMainFrame()) {
            for (let breakpoints of this._domBreakpointURLMap.values())
                breakpoints.forEach((breakpoint) => { breakpoint.domNodeIdentifier = null; });

            this._domBreakpointFrameIdentifierMap.clear();
        } else
            this._detachBreakpointsForFrame(frame);

        this._speculativelyResolveBreakpoints();
    }

    _nodeInserted(event)
    {
        let node = event.data.node;
        if (node.nodeType() !== Node.ELEMENT_NODE || !node.ownerDocument)
            return;

        let url = node.ownerDocument.documentURL;
        let breakpoints = this._domBreakpointURLMap.get(url);
        if (!breakpoints)
            return;

        for (let breakpoint of breakpoints) {
            if (breakpoint.domNodeIdentifier)
                continue;

            if (breakpoint.path !== node.path())
                continue;

            this._resolveDOMBreakpoint(breakpoint, node.id);
        }
    }

    _nodeRemoved(event)
    {
        let node = event.data.node;
        if (node.nodeType() !== Node.ELEMENT_NODE || !node.ownerDocument)
            return;

        let domBreakpointNodeIdentifierMap = this._domBreakpointFrameIdentifierMap.get(node.frameIdentifier);
        if (!domBreakpointNodeIdentifierMap)
            return;

        let breakpoints = domBreakpointNodeIdentifierMap.get(node.id);
        if (!breakpoints)
            return;

        domBreakpointNodeIdentifierMap.delete(node.id);

        if (!domBreakpointNodeIdentifierMap.size)
            this._domBreakpointFrameIdentifierMap.delete(node.frameIdentifier);

        for (let breakpoint of breakpoints)
            breakpoint.domNodeIdentifier = null;
    }
};

WebInspector.DOMDebuggerManager.Event = {
    DOMBreakpointAdded: "dom-debugger-manager-dom-breakpoint-added",
    DOMBreakpointRemoved: "dom-debugger-manager-dom-breakpoint-removed",
};
