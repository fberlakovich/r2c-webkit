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

class TimeLabel extends LayoutNode
{

    constructor(timeControl)
    {
        super(`<div class="time-label"></div>`);

        this.value = 0;
        this._timeControl = timeControl;
    }

    // Public

    get value()
    {
        return this._value;
    }

    set value(value)
    {
        if (value === this._value)
            return;

        this._value = value;
        this.markDirtyProperty("value");
    }

    // Protected

    commitProperty(propertyName)
    {
        if (propertyName === "value") {
            this.element.textContent = this._formattedTime();
            if (this._timeControl)
                this._timeControl.updateScrubberLabel();
        }
        else
            super.commitProperty(propertyName);
    }

    // Private

    _formattedTime()
    {
        const value = this._value || 0;
        const time = formatTimeByUnit(value);
        const timeStrings = [time.minutes, time.seconds];
        if (time.hours > 0 || (this._timeControl && this._timeControl.useSixDigitsForTimeLabels))
            timeStrings.unshift(time.hours);

        const sign = value < 0 ? "-" : "";
        return sign + timeStrings.map(x => `00${x}`.slice(-2)).join(":");
    }

}
