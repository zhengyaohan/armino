/**
 * This test receives a multicast UDP request. Nodes respond using unicast UDP and include the
 * payload provided by the caller.**/

var dgram = require('dgram');
var threadMac = require('./threadMacInfo');
var client = dgram.createSocket('udp6');

client.on('listening', function () {
    var address = client.address();
    console.log('UDP Server listening on ' + address.address + ":" + address.port);
});

client.on('message', function (message, remote) {
    console.log('Received message from: ' + remote.address + ':' + remote.port +' - ' + message);
    client.send(message, 0, message.length, remote.port, remote.address, function(err, bytes) {
        if (err) throw err;
    });
   console.log('Replied to: ' + remote.address + ':' + remote.port +' - '+message);	
});

client.bind(19085,'::');
threadMac.getThreadMacValues((data) => {
	console.log(data);
});

