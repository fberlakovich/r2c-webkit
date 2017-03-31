'use strict';

if (self.importScripts) {
    self.importScripts('../resources/testharness.js');
}

test(() => {
    const methods = ['constructor', 'respond', 'respondWithNewView'];
    const properties = methods.concat(['view']).sort();

    let controller;

    // FIXME: Remove next line when bug https://bugs.webkit.org/show_bug.cgi?id=167697
    // is fixed. For the moment, so that test may pass, we have to insert a reference
    // to Uint8Array here (otherwise, the private variable cannot be resolved).
    const d = new Uint8Array(1);

    // Specifying autoAllocateChunkSize and calling read() are steps that allow
    // getting a ReadableStreamBYOBRequest returned instead of undefined. The
    // purpose here is just to get such an object.
    const rs = new ReadableStream({
        autoAllocateChunkSize: 128,
        start: function(c) {
            controller = c;
        },
        type: "bytes"
    });

    rs.getReader().read();
    const byobReq = controller.byobRequest;

    const proto = Object.getPrototypeOf(byobReq);

    assert_array_equals(Object.getOwnPropertyNames(proto).sort(), properties);

    for (const m of methods) {
        const propDesc = Object.getOwnPropertyDescriptor(proto, m);
        assert_equals(propDesc.enumerable, false, 'method should be non-enumerable');
        assert_equals(propDesc.configurable, true, 'method should be configurable');
        assert_equals(propDesc.writable, true, 'method should be writable');
        assert_equals(typeof byobReq[m], 'function', 'should have be a method');
    }

    const viewPropDesc = Object.getOwnPropertyDescriptor(proto, 'view');
    assert_equals(viewPropDesc.enumerable, false, 'view should be non-enumerable');
    assert_equals(viewPropDesc.configurable, true, 'view should be configurable');
    assert_not_equals(viewPropDesc.get, undefined, 'view should have a getter');
    assert_equals(viewPropDesc.set, undefined, 'view should not have a setter');

    assert_equals(byobReq.constructor.length, 2, 'constructor has 2 parameters');
    assert_equals(byobReq.respond.length, 1, 'respond has 1 parameter');
    assert_equals(byobReq.respondWithNewView.length, 1, 'respondWithNewView has 1 parameter');

}, 'ReadableStreamBYOBRequest instances should have the correct list of properties');

test(function() {
    let controller;

    const rs = new ReadableStream({
        start: function(c) {
            controller = c;
        },
        type: "bytes"
    });

    assert_equals(controller.byobRequest, undefined, "by default byobRequest should be undefined");
}, "By default, byobRequest should be undefined");

test(function() {

    let controller;
    const autoAllocateChunkSize = 128;
    const rs = new ReadableStream({
        autoAllocateChunkSize,
        start: function(c) {
            controller = c;
        },
        type: "bytes"
    });

    rs.getReader().read();
    const byobReq = controller.byobRequest;

    assert_equals(byobReq.view.length, autoAllocateChunkSize, "byobRequest length should be equal to autoAllocateChunkSize value")

}, "byobRequest.view length should be equal to autoAllocateChunkSize")

test(function() {

    let controller;
    const rs = new ReadableStream({
        autoAllocateChunkSize: 16,
        start: function(c) {
            controller = c;
        },
        type: "bytes"
    });

    rs.getReader().read();
    const byobReq = controller.byobRequest;

    assert_throws(new TypeError("Can only call ReadableStreamBYOBRequest.respond on instances of ReadableStreamBYOBRequest"),
        function() { byobReq.respond.apply(rs, 1); });

}, "Calling respond() with a this object different from ReadableStreamBYOBRequest should throw a TypeError");

test(function() {

    let controller;
    const rs = new ReadableStream({
        autoAllocateChunkSize: 16,
        start: function(c) {
            controller = c;
        },
        type: "bytes"
    });

    rs.getReader().read();
    const byobReq = controller.byobRequest;

    assert_throws(new RangeError("bytesWritten has an incorrect value"),
        function() { byobReq.respond(-1); });
}, "Calling respond() with a negative bytesWritten value should throw a RangeError");

test(function() {

    let controller;
    const rs = new ReadableStream({
        autoAllocateChunkSize: 16,
        start: function(c) {
            controller = c;
        },
        type: "bytes"
    });

    rs.getReader().read();
    const byobReq = controller.byobRequest;

    assert_throws(new RangeError("bytesWritten has an incorrect value"),
        function() { byobReq.respond("abc"); });
}, "Calling respond() with a bytesWritten value which is not a number should throw a RangeError");

test(function() {

    let controller;
    const rs = new ReadableStream({
        autoAllocateChunkSize: 16,
        start: function(c) {
            controller = c;
        },
        type: "bytes"
    });

    rs.getReader().read();
    const byobReq = controller.byobRequest;

    assert_throws(new RangeError("bytesWritten has an incorrect value"),
        function() { byobReq.respond(Number.POSITIVE_INFINITY); });
}, "Calling respond() with a positive infinity bytesWritten value should throw a RangeError");

test(function() {

    let controller;
    const rs = new ReadableStream({
        autoAllocateChunkSize: 16,
        start: function(c) {
            controller = c;
        },
        type: "bytes"
    });

    rs.getReader().read();
    const byobReq = controller.byobRequest;
    controller.close();

    assert_throws(new TypeError("bytesWritten is different from 0 even though stream is closed"),
        function() { byobReq.respond(1); });
}, "Calling respond() with a bytesWritten value different from 0 when stream is closed should throw a TypeError");

test(function() {

    let controller;
    const rs = new ReadableStream({
        autoAllocateChunkSize: 16,
        start: function(c) {
            controller = c;
        },
        type: "bytes"
    });

    // FIXME: When ReadableStreamBYOBReader is implemented, another test (or even several ones)
    // based on this one should be added so that reader's readIntoRequests attribute is not empty
    // and currently unreachable code is reached.
    rs.getReader().read();
    const byobReq = controller.byobRequest;
    controller.close();
    byobReq.respond(0);

}, "Calling respond() with a bytesWritten value of 0 when stream is closed should succeed");

test(function() {

    let controller;
    const rs = new ReadableStream({
        autoAllocateChunkSize: 16,
        start: function(c) {
            controller = c;
        },
        type: "bytes"
    });

    rs.getReader().read();
    const byobReq = controller.byobRequest;
    assert_throws(new RangeError("bytesWritten value is too great"),
        function() { byobReq.respond(17); });

}, "Calling respond() with a bytesWritten value greater than autoAllocateChunkSize should fail");

promise_test(function() {

    const rs = new ReadableStream({
        autoAllocateChunkSize: 16,
        pull: function(controller) {
            const br = controller.byobRequest;
            br.view[0] = 1;
            br.view[1] = 2;
            br.respond(2);
        },
        type: "bytes"
    });

    return rs.getReader().read().then(result => {
        assert_equals(result.value.byteLength, 2);
        assert_equals(result.value.byteOffset, 0);
        assert_equals(result.value.buffer.byteLength, 16);
        assert_equals(result.value[0], 1);
        assert_equals(result.value[1], 2);
    });
}, "Calling respond() with a bytesWritten value lower than autoAllocateChunkSize should succeed");

// FIXME: when ReadableStreamBYOBReader is implemented, add tests with elementSize different from 1
// so that more code can be covered.

if (!self.importScripts) {
    // Test only if not Worker.
    const CloneArrayBuffer = internals.cloneArrayBuffer.bind(internals);

    test(function() {
        const typedArray = new Uint8Array([3, 5, 7]);
        const clonedBuffer = CloneArrayBuffer(typedArray.buffer, 1, 1);
        const otherArray = new Uint8Array(clonedBuffer);
        assert_equals(otherArray.byteLength, 1);
        assert_equals(otherArray.byteOffset, 0);
        assert_equals(otherArray.buffer.byteLength, 1);
        assert_equals(otherArray[0], 5);
        // Check that when typedArray is modified, otherArray is not modified.
        typedArray[1] = 0;
        assert_equals(otherArray[0], 5);
    }, "Test cloneArrayBuffer implementation");
}

done();
