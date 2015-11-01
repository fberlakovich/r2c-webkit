function Rotater(rotateInterval)
{
    this._timeDelta = 0;
    this._rotateInterval = rotateInterval;
}

Rotater.prototype =
{
    get interval()
    {
        return this._rotateInterval;
    },
    
    next: function(timeDelta)
    {
        this._timeDelta = (this._timeDelta + timeDelta) % this._rotateInterval;
    },
    
    degree: function()
    {
        return (360 * this._timeDelta) / this._rotateInterval;
    },
    
    rotateZ: function()
    {
        return "rotateZ(" + Math.floor(this.degree()) + "deg)";
    },
    
    rotate: function(center)
    {
        return "rotate(" + Math.floor(this.degree()) + ", " + center.x + "," + center.y + ")";
    }
}

function Stage(element, options)
{
    this.element = element;
}

Stage.prototype =
{
    get size()
    {
        var styles = window.getComputedStyle(this.element);
        var padding = new Insets(
            parseFloat(styles.paddingTop),
            parseFloat(styles.paddingRight),
            parseFloat(styles.paddingBottom),
            parseFloat(styles.paddingTop));
        return new Point(this.element.clientWidth - padding.width, this.element.clientHeight - padding.height);
    },
    
    random: function(min, max)
    {
        return (Math.random() * (max - min)) + min;
    },
    
    randomBool: function()
    {
        return !!Math.round(this.random(0, 1));
    },

    randomInt: function(min, max)
    {
        return Math.round(this.random(min, max));
    },
    
    randomPosition: function(maxPosition)
    {
        return new Point(this.randomInt(0, maxPosition.x), this.randomInt(0, maxPosition.y));
    },
    
    randomSquareSize: function(min, max)
    {
        var side = this.random(min, max);
        return new Point(side, side);
    },
    
    randomVelocity: function(maxVelocity)
    {
        return this.random(maxVelocity / 8, maxVelocity);
    },
    
    randomAngle: function()
    {
        return this.random(0, Math.PI * 2);
    },
    
    randomColor: function()
    {
        var min = 32;
        var max = 256 - 32;
        return "#"
            + this.randomInt(min, max).toString(16)
            + this.randomInt(min, max).toString(16)
            + this.randomInt(min, max).toString(16);
    },
    
    randomRotater: function()
    {
        return new Rotater(this.random(1000, 10000));
    },

    tune: function()
    {
        throw "Not implemented";
    },
    
    animate: function()
    {
        throw "Not implemented";
    },
    
    clear: function()
    {
        return this.tune(-this.tune(0));
    }
}

function StageAnimator(benchmark)
{
    Animator.call(this, benchmark);
};

StageAnimator.prototype = Object.create(Animator.prototype);
StageAnimator.prototype.constructor = StageAnimator;

StageAnimator.prototype.animate = function()
{
    if (!Animator.prototype.animate.call(this))
        return false;
    this._benchmark._stage.animate(this.timeDelta());
    return true;
}

function StageBenchmark(suite, test, options, recordTable, progressBar)
{
    Benchmark.call(this, options);
    
    var element = document.getElementById("stage");
    element.setAttribute("width", document.body.offsetWidth);
    element.setAttribute("height", document.body.offsetHeight);
    
    this._stage = this.createStage(element);
    this._animator = this.createAnimator();
    this._suite = suite;
    this._test = test;
    this._recordTable = recordTable;
    this._progressBar = progressBar;
}

StageBenchmark.prototype = Object.create(Benchmark.prototype);
StageBenchmark.prototype.constructor = StageBenchmark;

StageBenchmark.prototype.createStage = function(element)
{
    return new Stage(element, this._options);
}

StageBenchmark.prototype.createAnimator = function()
{
    return new StageAnimator(this);
}

StageBenchmark.prototype.tune = function(count)
{
    return this._stage.tune(count);
}

StageBenchmark.prototype.clear = function()
{
    return this._stage.clear();
}

StageBenchmark.prototype.showResults = function(message, progress)
{
    if (!this._recordTable || !this._progressBar || !this._test)
        return;

    if (this.options["show-running-results"])
        this._recordTable.showRecord(this._test.name, message, this._sampler.toJSON(true, false));

    this._progressBar.setPos(progress);
}
