// http://tim.dysinger.net/posts/2013-09-16-getting-started-with-nanomsg.html

var assert = require('assert');
var should = require('should');
var nano = require('../');

var test = require('tape');

test('inproc socket survey', function (t) {
    t.plan(4);

    var sur = nano.socket('surveyor');
    var rep1 = nano.socket('respondent');
    var rep2 = nano.socket('respondent');
    var rep3 = nano.socket('respondent');

    var addr = 'inproc://survey';
    var msg1 = 'knock knock';
    var msg2 = "who's there?";

    sur.bindSync(addr);
    rep1.connectSync(addr);
    rep2.connectSync(addr);
    rep3.connectSync(addr);

    function answer (buf) {
        this.send(msg2);
    }

    rep1.on('message', answer);
    rep2.on('message', answer);
    rep3.on('message', answer);

    var count = 0;
    sur.survey(msg1, function (res) {
        t.ok(res.length == 3);
        t.ok(res[0] == msg2);
        t.ok(res[1] == msg2);
        t.ok(res[2] == msg2);

        sur.close();
        rep1.close();
        rep2.close();
        rep3.close();
    })
});
