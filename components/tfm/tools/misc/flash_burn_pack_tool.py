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

# image head:
#   size
#   offset
#   hash(20 bytes)

# image end:
#   0x12345678
#   0xabcdef01
#   0x00000000
#   0x00000000
#   0x00000000
#   0x00000000
#   0x00000000

# out.bin
# image head (_1)
# image head (_2)
# ...
# image head (_n)
# image end

import sys
import os
import struct
import hashlib
import string

image_info = []
image_header = []
image_data = []
OUTFILE = "./out.bin"
class ImageInfo:
    def __init__(self, name, offset):
        self.name = name
        self.offset = offset
        self.size = 0
        self.hash = 0

def calc_sha1(filename):
    fd = open(filename, "rb")
    sha1obj = hashlib.sha1()
    sha1obj.update(fd.read())
    hash = sha1obj.hexdigest()
    fd.close()
    return hash

def print_usage():
    print("Usage")
    print("      python3 image_package.py  [file1] [hex_offset1] [file2] [hex_offset2] [file3] [hex_offset3] [...]")
    print("Example:")
    print("      python3 image_package.py  ./secure_boot.bin 0x1000 ./spe.bin 0x5000 ./zephyr.bin 0x10000")

def arg_parser(argv):
    #print("length:", len(argv))
    for idx in range(len(argv)):
        if idx == 0:
            continue
        if idx % 2 == 0:
            #print("idx", idx, "last", argv[idx - 1], "current:", argv[idx])
            image_info.append(ImageInfo(argv[idx - 1], int(argv[idx], 16)))

def check_image(imageinfo):
    # check file exist
    for i in image_info:
        #print("Image name", i.name, "offset %#x" % i.offset)
        # check if file exist
        if not (os.path.exists(i.name)):
            print("File ", i.name, "not exist!")
            return False
        # get file size
        i.size = os.path.getsize(i.name)
        i.hash = calc_sha1(i.name)
        # pad hash to 20 bytes if not 20 bytes
        i.hash = '0' * (20 * 2 - len(i.hash)) + i.hash
    # check file overlap
    for i in image_info:
        #print("Image name", i.name, "offset %#x" % i.offset, "size %#x" % i.size)
        for j in image_info:
            # skip ourself
            if i == j:
                continue
            # check overlap
            if not ((i.offset >= j.offset + j.size) or (i.offset + i.size <= j.offset)):
                print("File ", i.name, j.name, "overlap!")
                print(i.name, i.offset, i.size)
                print(j.name, j.offset, j.size)
                return False
    return True

def package(imageinfo):
    for i in imageinfo:
        fd = open(i.name, "rb")
        image_data.append(fd.read())
        image_header.append(int(i.offset))
        image_header.append(int(i.size))

        for t in range(0, len(i.hash), 8):
            data = i.hash[t:t + 8]
            # bn to ln
            #data = data.decode('hex')[::-1].encode('hex_codec')
            data = data[6] + data[7] + data[4] + data[5] + data[2] + data[3] + data[0] + data[1]
            image_header.append(int(data,base=16))

        #image_header.append(string.atol(i.hash[0:8], base=16))
        #image_header.append(string.atol(i.hash[8:16], base=16))
        #image_header.append(string.atol(i.hash[16:24], base=16))
        #image_header.append(string.atol(i.hash[24:32], base=16))
        #image_header.append(string.atol(i.hash[32:40], base=16))
        fd.close()

    image_header.append(0x12345678)
    image_header.append(0xabcdef01)
    # hash all 0
    image_header.append(0x00000000)
    image_header.append(0x00000000)
    image_header.append(0x00000000)
    image_header.append(0x00000000)
    image_header.append(0x00000000)
    
    # create out file
    outfd = open(OUTFILE, "wb+")
    outfd.truncate(0)
    for h in image_header:
        outfd.write(struct.pack("I", h))
    for d in image_data:
        outfd.write(d)
    outfd.close()
    return os.path.getsize(OUTFILE)
# return the offset element of key
def take_offset(imageinfo):
    return imageinfo.offset
    
if __name__ == "__main__":
    if len(sys.argv) <= 1:
        print_usage()
        exit(-1)
    arg_parser(sys.argv)
    valid = check_image(image_info)
    if not valid:
        print_usage()
        exit(-1)
    image_info.sort(key=take_offset)
    print("===============================")
    for i in image_info:
        print("Image name: " + i.name +
            " offset: " + (hex)(i.offset) +
            " size :" + (str)(i.size) + "(" + (hex)(i.size) + ")" +
            " sha1sum " + i.hash)
    print("===============================")
    outsize = package(image_info)
    print("Out File: " + OUTFILE + " Size: " + str(outsize))
    exit(0)
