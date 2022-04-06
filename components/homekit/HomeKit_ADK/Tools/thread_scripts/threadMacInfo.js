var exec = require('child_process').exec;

function execute(command, callback){
    exec(command, function(error, stdout, stderr){ callback(stdout); });
};

function parseData(macCountersText) {
        var mainKeyIndex = macCountersText.indexOf('NCP:Counter:AllMac');

        if (mainKeyIndex == -1) {
                return {};
        }

        var openSquareBracketIndex = macCountersText.indexOf('[', mainKeyIndex);
        if (openSquareBracketIndex == -1) {
                return {};
        }

        var closeSquareBracketIndex = macCountersText.indexOf(']', mainKeyIndex);
        if (closeSquareBracketIndex == -1) {
                return {};
        }

        var values = macCountersText.substring(openSquareBracketIndex + 3, closeSquareBracketIndex);
        values = values.replace(/(\")/gm, '');
        var keyValues = values.split('\t');

        var macData = {};
        for (i = 0; i < keyValues.length; i++) {
                let kv = keyValues[i];
                let kvSplit = kv.split('=');
                let key = kvSplit[0].replace(/\s/g,'');
                let value = kvSplit[1].replace(/\s/g,'');
                macData[key] = value;
        }
        return macData;
}

function getThreadMacValues(callback) {
	execute("sudo wpanctl get", (out) => {
	   callback(parseData(out));
	});
};

module.exports = {
	getThreadMacValues:getThreadMacValues
}


/* This is a list of the fields included in the object
TxTotal = TxUnicast + TxBroadcast
TxUnicast = TxAckRequested

NCP:Counter:AllMac = [
    "TxTotal              = 1126"
    "TxUnicast            = 289"
    "TxBroadcast          = 837"
    "TxAckRequested       = 289"
    "TxAcked              = 281"
    "TxNoAckRequested     = 837"
    "TxData               = 1126"
    "TxDataPoll           = 0"
    "TxBeacon             = 0"
    "TxBeaconRequest      = 0"
    "TxOther              = 0"
    "TxRetry              = 106"
    "TxErrCca             = 67"
    "TxErrAbort           = 0"
    "TxErrBusyChannel     = 6"
    "RxTotal              = 1651"
    "RxUnicast            = 287"
    "RxBroadcast          = 1055"
    "RxData               = 1320"
    "RxDataPoll           = 0"
    "RxBeacon             = 1"
    "RxBeaconRequest      = 0"
    "RxOther              = 0"
    "RxAddressFiltered    = 0"
    "RxDestAddrFiltered   = 259"
    "RxDuplicated         = 21"
    "RxErrNoFrame         = 6"
    "RxErrUnknownNeighbor = 0"
    "RxErrInvalidSrcAddr  = 0"
    "RxErrSec             = 0"
    "RxErrFcs             = 41"
    "RxErrOther           = 3"
]
*/
