// https://github.com/chuckremes/nn-core/blob/master/spec/nn_bind_spec.rb

var assert = require('assert');
var should = require('should');
var nano = require('../');
var nn = nano._bindings;

var test = require('tape');

function expectSuccess(err, sock) {

}

test('bind without error for valid INPROC address', function (t) {
    t.plan(1);

    var sock = nano.socket('pub');
    sock.bind('inproc://some_address', function (err) {
        if (err) {
            t.fail('bind failed with ' + err);
        } else {
            t.pass('bind succeeded');
        }
        sock.close();
    });


});

test('bind without error for valid IPC address', function (t) {
    t.plan(1);

    var sock = nano.socket('pub');
    sock.bind('ipc:///tmp/some_address.ipc', function (err) {
        if (err) {
            t.fail('bind failed with ' + err);
        } else {
            t.pass('bind succeeded');
        }
        sock.close();
    });

});

test('bind without error for valid TCP address', function (t) {
    t.plan(1);

    var sock = nano.socket('pub');
    sock.bind('tcp://127.0.0.1:5555', function (err) {
        if (err) {
            t.fail('error ' + err);
        } else {
            t.pass('success');
        }
        sock.close();
    });

});

test('bind with error for invalid INPROC address', function (t) {
    t.plan(2);

    var sock = nano.socket('pub');
    sock.bind('inproc:/missing_first_slash', function (err) {
        if (!err) {
            t.fail('error missing');
        } else {
            t.equal(err.toString(), 'Error: Invalid argument');
            t.pass('success, error ' + err);
        }
        sock.close();
    });
});

test('bind with error for invalid INPROC address (too long)', function (t) {
    t.plan(2);

    var sock = nano.socket('pub');

    var addr = new Array(nn.NN_SOCKADDR_MAX + 1).join('a');
    sock.bind('inproc://' + addr, function (err) {
        if (!err) {
            t.fail('error missing');
        } else {
            t.equal(err.toString(), 'Error: File name too long');
            t.pass('success, error ' + err);
        }
        sock.close();
    });
});

test('bind with error for invalid IPC address', function (t) {
    t.plan(2);

    var sock = nano.socket('pub');

    sock.bind('ipc:/missing_first_slash', function (err) {
        if (!err) {
            t.fail('error missing');
        } else {
            t.equal(err.toString(), 'Error: Invalid argument');
            t.pass('success, error ' + err);
        }
        sock.close();
    });
});

test('bind with error for invalid TCP address (missing address)', function (t) {
    t.plan(2);

    var sock = nano.socket('pub');

    sock.bind('tcp://', function (err) {
        if (!err) {
            t.fail('error missing');
        } else {
            t.equal(err.toString(), 'Error: Invalid argument');
            t.pass('success, error ' + err);
        }
        sock.close();
    });
});

test('bind with error for invalid TCP address (non-numeric port)', function (t) {
    t.plan(2);

    var sock = nano.socket('pub');


    sock.bind('tcp://127.0.0.1:port', function (err) {
        if (!err) {
            t.fail('error missing');
        } else {
            t.equal(err.toString(), 'Error: Invalid argument');
            t.pass('success, error ' + err);
        }
        sock.close();
    });
});

test('bind with error for invalid TCP address (port out of range)', function (t) {
    t.plan(2);

    var sock = nano.socket('pub');

    sock.bind('tcp://127.0.0.1:65536', function (err) {
        if (!err) {
            t.fail('error missing');
        } else {
            t.equal(err.toString(), 'Error: Invalid argument');
            t.pass('success, error ' + err);
        }
        sock.close();
    });
});

test('bind with error for unsupported transport', function (t) {
    t.plan(2);

    var sock = nano.socket('pub');

    sock.bind('zmq://127.0.0.1:6000', function (err) {
        if (!err) {
            t.fail('error missing');
        } else {
            t.equal(err.toString(), 'Error: Protocol not supported');
            t.pass('success, error ' + err);
        }
        sock.close();
    });
});

test('bind with error for TCP on non-existent device', function (t) {
    t.plan(2);

    var sock = nano.socket('pub');

    sock.bind('tcp://eth99:555', function (err) {
        if (!err) {
            t.fail('error missing');
        } else {
            t.equal(err.toString(), 'Error: No such device');
            t.pass('success, error ' + err);
        }
        sock.close();
    });
});

test('bind with error for INPROC rebind to existing endpoint', function (t) {
    t.plan(2);

    var sock = nano.socket('pub');
    var sock2 = nano.socket('pub');
    var addr = 'inproc://some_endpoint';

    sock2.on('error', function (err) {
        t.ok(err, 'error thrown on INPROC multiple binds');
        t.equal(nn.Errno(), nn.EADDRINUSE);
        sock.close();
        sock2.close();
    });

    sock.bind(addr, function (err) {
        if (err) {
            t.fail('bind failed with ' + err);
        }
        sock2.bind(addr, function (err) {
            if (!err) {
                t.fail('error missing');
            } else {
                t.equal(err.toString(), 'Error: Address already in use');
                t.pass('success, error ' + err);
            }
            sock.close();
            sock2.close();
        });
    });
});


