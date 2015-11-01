function BenchmarkRunnerState(suites)
{
    this._suites = suites;
    this._suiteIndex = -1;
    this._testIndex = 0;
    this.next();
}

BenchmarkRunnerState.prototype.currentSuite = function()
{
    return this._suites[this._suiteIndex];
}

BenchmarkRunnerState.prototype.currentTest = function()
{
    var suite = this.currentSuite();
    return suite ? suite.tests[this._testIndex] : null;
}

BenchmarkRunnerState.prototype.isFirstTest = function()
{
    return !this._testIndex;
}

BenchmarkRunnerState.prototype.next = function()
{
    this._testIndex++;

    var suite = this._suites[this._suiteIndex];
    if (suite && this._testIndex < suite.tests.length)
        return this;

    this._testIndex = 0;
    do {
        this._suiteIndex++;
    } while (this._suiteIndex < this._suites.length && this._suites[this._suiteIndex].disabled);

    return this;
}

BenchmarkRunnerState.prototype.prepareCurrentTest = function(runner, frame)
{
    var suite = this.currentSuite();
    var test = this.currentTest();
    var promise = new SimplePromise;
    frame.onload = function() {
        suite.prepare(runner, frame.contentWindow, frame.contentDocument).then(function(result) { promise.resolve(result); });
    }
    frame.src = "../tests/" + test.url;
    return promise;
}

function BenchmarkRunner(suites, frameContainer, client)
{
    this._suites = suites;
    this._client = client;
    this._frameContainer = frameContainer;
}

BenchmarkRunner.prototype.waitForElement = function(selector)
{
    var promise = new SimplePromise;
    var contentDocument = this._frame.contentDocument;

    function resolveIfReady() {
        var element = contentDocument.querySelector(selector);
        if (element)
            return promise.resolve(element);
        setTimeout(resolveIfReady, 50);
    }

    resolveIfReady();
    return promise;
}

BenchmarkRunner.prototype._appendFrame = function()
{
    var frame = document.createElement("iframe");
    frame.setAttribute("scrolling", "no");

    if (this._client && this._client.willAddTestFrame)
        this._client.willAddTestFrame(frame);

    this._frameContainer.insertBefore(frame, this._frameContainer.firstChild);
    this._frame = frame;
    return frame;
}

BenchmarkRunner.prototype._removeFrame = function()
{
    if (this._frame) {
        this._frame.parentNode.removeChild(this._frame);
        this._frame = null;
    }
}

BenchmarkRunner.prototype._runTestAndRecordResults = function(state)
{
    var promise = new SimplePromise;
    var suite = state.currentSuite();
    var test = state.currentTest();
    
    if (this._client && this._client.willRunTestTest)
        this._client.willRunTest(suite, test);

    var contentWindow = this._frame.contentWindow;
    var self = this;

    suite.run(contentWindow, test, this._client.options, this._client.recordTable, this._client.progressBar).then(function(sampler) {
        var samplers = self._suitesSamplers[suite.name] || [];
        
        samplers[test.name] = sampler;
        self._suitesSamplers[suite.name] = samplers;

        if (self._client && self._client.didRunTest)
            self._client.didRunTest(suite, test);
        
        state.next();
        if (state.currentSuite() != suite)
            self._removeFrame();
        promise.resolve(state);
    });
    
    return promise;
}

BenchmarkRunner.prototype.step = function(state)
{
    if (!state) {
        state = new BenchmarkRunnerState(this._suites);
        this._suitesSamplers = {};
    }

    var suite = state.currentSuite();
    if (!suite) {
        this._finalize();
        var promise = new SimplePromise;
        promise.resolve();
        return promise;
    }

    if (state.isFirstTest()) {
        this._masuredValuesForCurrentSuite = {};
        this._appendFrame();
    }

    return state.prepareCurrentTest(this, this._frame).then(function(prepareReturnValue) {
        return this._runTestAndRecordResults(state);
    }.bind(this));
}

BenchmarkRunner.prototype.runAllSteps = function(startingState)
{
    var nextCallee = this.runAllSteps.bind(this);
    this.step(startingState).then(function(nextState) {
        if (nextState)
            nextCallee(nextState);
    });
}

BenchmarkRunner.prototype.runMultipleIterations = function()
{
    var self = this;
    var currentIteration = 0;

    this._runNextIteration = function() {
        currentIteration++;
        if (currentIteration < self._client.iterationCount)
            self.runAllSteps();
        else if (this._client && this._client.didFinishLastIteration)
            self._client.didFinishLastIteration();
    }

    if (self._client && self._client.willStartFirstIteration)
        self._client.willStartFirstIteration();

    self.runAllSteps();
}

BenchmarkRunner.prototype._finalize = function()
{
    this._removeFrame();
        
    if (this._client && this._client.didRunSuites)
        this._client.didRunSuites(this._suitesSamplers);

    if (this._runNextIteration)
        this._runNextIteration();
}
