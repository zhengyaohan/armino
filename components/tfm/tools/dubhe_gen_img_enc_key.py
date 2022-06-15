#!/usr/bin/python

# Copyright (C), 2018-2018, Arm Technology (China) Co., Ltd.
# All rights reserved
#
# The content of this file or document is CONFIDENTIAL and PROPRIETARY
# to Arm Technology (China) Co., Ltd. It is subject to the terms of a
# License Agreement between Licensee and Arm Technology (China) Co., Ltd
# restricting among other things, the use, reproduction, distribution
# and transfer.  Each of the embodiments, including this information and,,
# any derivative work shall retain this copyright notice.

# -*- coding: UTF-8 -*-

import sys
import os
import struct
import hashlib
import string
from Crypto.Cipher import AES

def print_usage():
    print("Usage")
    print(
        "      python3 dubhe_gen_img_enc_key.py  [model key] [key blob]")
    print("Example:")
    print("      python3 dubhe_gen_img_enc_key.py  3864145fb6d16f0e6c39ea191dbc1720 00112233445566778899aabbccddeeff")


ek2 = [0x49, 0xC0, 0x68, 0x83, 0xF2, 0x98, 0x63, 0x7D,
       0x8D, 0x2F, 0x1B, 0x3C, 0xF5, 0x5D, 0x1A, 0xC4]

ek3 = [0x35, 0x09, 0x32, 0x0F, 0xF9, 0x69, 0x91, 0x4F,
       0x2F, 0x76, 0x9F, 0xBC, 0x1E, 0xF1, 0x41, 0x67]

model_key_size = 16
ek1_size_array = [16, 32]


def _print_bytes(name, data):
    print(name)
    for i in range(0, len(data)):
        print("0x%02x " % data[i], end='')
    print("")


def aes_ecb_dec(key, datain):
    #_print_bytes("Key:", key)
    #_print_bytes("In:", datain)

    ctx = AES.new(key, AES.MODE_ECB)
    dataout = ctx.decrypt(datain)
    #_print_bytes("Out:", dataout)
    return dataout


def _convert_hex_str_to_byte_array(hex_str):
    hex_data_array = []
    if len(hex_str) & 1:
        return False, [0]
    for i in range(0, len(hex_str), 2):
        data = hex_str[i:i + 2]
        try:
            hex_data_array.append(int(data, base=16))
        except:
            return False, [0]
    return True, hex_data_array


def _convert_bytes_to_hex_str(bytes):
    bytes_list = list(bytes)
    bytes_str = ''.join("%02X" % e for e in bytes_list)
    return bytes_str


def gen_img_enc_key(model_key, key_blob):
    ret, key_blob_byte_array = _convert_hex_str_to_byte_array(key_blob)
    if False == ret:
        print("Bad Key blob, not hex string!")
        exit(-1)

    ret, model_key_byte_array = _convert_hex_str_to_byte_array(model_key)
    if False == ret:
        print("Bad model key, not hex string!")
        exit(-1)

    tmp_len = len(key_blob)
    tmp_len = tmp_len / 2
    if tmp_len not in ek1_size_array:
        print("Bad key blob size, only support 128 or 256 bits!")
        exit(-1)

    tmp_len = len(model_key)
    tmp_len = tmp_len / 2
    if tmp_len != model_key_size:
        print("Bad model key size, MUST be 128 bits!")
        exit(-1)

    tmp_out = aes_ecb_dec(bytes(model_key_byte_array), bytes(ek3))
    tmp_out = aes_ecb_dec(tmp_out, bytes(ek2))
    tmp_out = aes_ecb_dec(tmp_out, bytes(key_blob_byte_array))
    return tmp_out


if __name__ == "__main__":
    if sys.version_info < (3, 0):
        print('Please use Python3 !')
        exit
    if len(sys.argv) < 3:
        print_usage()
        exit(-1)

    img_enc_key = gen_img_enc_key(sys.argv[1], sys.argv[2])
    print("Image encryption key:")
    print("    " + _convert_bytes_to_hex_str(img_enc_key))

    exit(0)
