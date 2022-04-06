#!/bin/bash

eval "sudo wpanctl leave"
eval "sudo wpanctl setprop Network:PANID 0xabcd"
eval "sudo wpanctl setprop Network:XPANID dead00beef00cafe"
eval "sudo wpanctl setprop Network:Key 00112233445566778899aabbccddeeff"
eval "sudo wpanctl form -c 11 OpenThread"
eval "sudo wpanctl config-gateway -d fd11:22::"
eval "sudo wpanctl setprop NCP:TXPower 8"
eval "sudo wpanctl status"
