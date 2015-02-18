// https://github.com/nanomsg/nanomsg/blob/master/tests/device.c
// test nn_device() with a PULLand PUSH socket (ie UNIDIRECTIONAL device)
//
// *NB* this is a standalone script because it calls nn_term() and therefore
// cannot coexist with any other nanomsg tests in the same process.

var assert = require('assert');
var should = require('should');
var nano = require('../../');

var test = require('tape');

test('create unidirectional device with two sockets', function (t) {
    t.plan(2);

    var r1 = nano.socket('pull', { raw: 1 });
    var r2 = nano.socket('push', { raw: 1 });

    var addr1 = 'inproc://device1';
    var addr2 = 'inproc://device2';
    var msg = "Hello";

    r1.bindSync(addr1);
    r2.bindSync(addr2);

    var d = nano.device(r1, r2);

    d.on('error', function (err) {
        t.ok(err, 'error was thrown when device collapsed:' + err);
        r1.close();
        r2.close();
    });


    var s1 = nano.socket('push');
    var s2 = nano.socket('pull');

    s1.connectSync(addr1);
    s2.connectSync(addr2);

    s2.on('message', function (buf) {
        t.equal(buf.toString(), msg);
        s1.close();
        s2.close();

        // nano.term() is the only way to shutdown a nano.device() !
        nano.term();
    });

    setTimeout(function () {
        s1.send(msg);
    }, 100);

});

