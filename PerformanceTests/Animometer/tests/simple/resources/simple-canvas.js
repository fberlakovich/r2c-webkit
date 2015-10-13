
function SimpleCanvasStage(element, options, canvasObject)
{
    Stage.call(this, element, options);
    this.context = this.element.getContext("2d");
    this.canvasObject = canvasObject;
    this._objects = [];
}
SimpleCanvasStage.prototype = Object.create(Stage.prototype);
SimpleCanvasStage.prototype.constructor = SimpleCanvasStage;
SimpleCanvasStage.prototype.tune = function(count)
{
    if (count == 0)
        return this._objects.length;

    if (count > 0) {
        for (var i = 0; i < count; ++i) {
            this._objects.push(new this.canvasObject(this));
        }
        return this._objects.length;
    }

    count = Math.min(-count, this._objects.length);
    this._objects.splice(-count, count);

    return this._objects.length;
}
SimpleCanvasStage.prototype.animate = function()
{
    var context = this.context;
    this._objects.forEach(function(object) {
        object.draw(context);
    });
}

function SimpleCanvasAnimator(benchmark)
{
    StageAnimator.call(this, benchmark);
    this._context = benchmark._stage.context;
}

SimpleCanvasAnimator.prototype = Object.create(StageAnimator.prototype);
SimpleCanvasAnimator.prototype.constructor = SimpleCanvasAnimator;
SimpleCanvasAnimator.prototype.animate = function()
{
    this._context.clearRect(0, 0, this._benchmark._stage.size.x, this._benchmark._stage.size.y);
    return StageAnimator.prototype.animate.call(this);
}


function SimpleCanvasBenchmark(suite, test, options, recordTable, progressBar) {
    options["gain"] = 6;
    options["addLimit"] = 100;
    options["removeLimit"] = 1000;
    StageBenchmark.call(this, suite, test, options, recordTable, progressBar);
}
SimpleCanvasBenchmark.prototype = Object.create(StageBenchmark.prototype);
SimpleCanvasBenchmark.prototype.constructor = SimpleCanvasBenchmark;
SimpleCanvasBenchmark.prototype.createAnimator = function() {
    return new SimpleCanvasAnimator(this);
}

