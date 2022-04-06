#!/bin/bash
eval "sudo wpanctl scan"
eval "sudo wpanctl leave"
eval "sudo wpanctl setprop Network:Key 00112233445566778899aabbccddeeff"
eval "sudo wpanctl join -p ABCD -x DEAD00BEEF00CAFE -c 25 OpenThread"
eval "sudo ip -6 addr add fd11:22::11/64 dev wpan0"
eval "sudo route add -A inet6 default gw fd11:22::"
eval "sudo wpanctl status"
