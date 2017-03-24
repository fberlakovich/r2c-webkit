/*
 * Copyright (C) 2016 Apple Inc. All Rights Reserved.
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

class MediaControls extends LayoutNode
{

    constructor({ width = 300, height = 150, layoutTraits = LayoutTraits.Unknown } = {})
    {
        super(`<div class="media-controls"></div>`);

        this._scaleFactor = 1;
        this._shouldCenterControlsVertically = false;

        this.width = width;
        this.height = height;
        this.layoutTraits = layoutTraits;

        this.startButton = new StartButton(this);

        this.playPauseButton = new PlayPauseButton(this);
        this.skipBackButton = new SkipBackButton(this);
        this.airplayButton = new AirplayButton(this);
        this.pipButton = new PiPButton(this);
        this.fullscreenButton = new FullscreenButton(this);

        this.statusLabel = new StatusLabel(this);
        this.timeControl = new TimeControl(this);

        this.controlsBar = new ControlsBar(this);

        this.airplayPlacard = new AirplayPlacard(this);
        this.invalidPlacard = new InvalidPlacard(this);
        this.pipPlacard = new PiPPlacard(this);

        this.showsStartButton = false;
    }

    // Public

    get layoutTraits()
    {
        return this._layoutTraits;
    }

    set layoutTraits(layoutTraits)
    {
        if (this._layoutTraits === layoutTraits)
            return;

        this._layoutTraits = layoutTraits;
        this.layoutTraitsDidChange();
    }

    get showsStartButton()
    {
        return !!this._showsStartButton;
    }

    set showsStartButton(flag)
    {
        if (this._showsStartButton === flag)
            return;

        this._showsStartButton = flag;
        this._invalidateChildren();
    }

    get usesLTRUserInterfaceLayoutDirection()
    {
        return this.element.classList.contains("uses-ltr-user-interface-layout-direction");
    }

    set usesLTRUserInterfaceLayoutDirection(flag)
    {
        this.element.classList.toggle("uses-ltr-user-interface-layout-direction", flag);
    }

    get scaleFactor()
    {
        return this._scaleFactor;
    }

    set scaleFactor(scaleFactor)
    {
        if (this._scaleFactor === scaleFactor)
            return;

        this._scaleFactor = scaleFactor;
        this.markDirtyProperty("scaleFactor");
    }

    get shouldCenterControlsVertically()
    {
        return this._shouldCenterControlsVertically;
    }

    set shouldCenterControlsVertically(flag)
    {
        if (this._shouldCenterControlsVertically === flag)
            return;

        this._shouldCenterControlsVertically = flag;
        this.markDirtyProperty("scaleFactor");
    }

    get placard()
    {
        return this.children[0] instanceof Placard ? this.children[0] : null;
    }

    get showsPlacard()
    {
        return !!this.placard;
    }

    showPlacard(placard)
    {
        const children = [placard];
        if (placard === this.airplayPlacard)
            children.push(this.controlsBar);

        this.children = children;
        this.layout();
    }

    hidePlacard()
    {
        if (this.showsPlacard)
            this.placard.remove();
        this._invalidateChildren();
    }

    fadeIn()
    {
        this.element.classList.add("fade-in");
    }

    // Protected

    commitProperty(propertyName)
    {
        if (propertyName === "scaleFactor") {
            const zoom = 1 / this._scaleFactor;
            // We want to maintain the controls at a constant device height.
            this.element.style.zoom = zoom;
            // We also want to optionally center them vertically compared to their container.
            this.element.style.top = this._shouldCenterControlsVertically ? `${(this.height / 2) * (zoom - 1)}px` : "auto"; 
        } else
            super.commitProperty(propertyName);
    }

    controlsBarVisibilityDidChange(controlsBar)
    {
        if (controlsBar.visible)
            this.layout();
    }

    controlsBarFadedStateDidChange()
    {
        if (this.delegate && typeof this.delegate.controlsBarFadedStateDidChange === "function")
            this.delegate.controlsBarFadedStateDidChange();
    }

    layoutTraitsDidChange()
    {
        // Implemented by subclasses as needed.
    }

    layout()
    {
        super.layout();

        if (this.showsPlacard) {
            this.placard.width = this.width;
            this.placard.height = this.height;
        }
    }

    // Private

    _invalidateChildren()
    {
        if (!this.showsPlacard)
            this.children = [this._showsStartButton ? this.startButton : this.controlsBar];
    }

}
