#!/usr/bin/env python3

import json
import re
import os
import argparse

import binascii


def dump(bin_file, output_file):
	with open(bin_file, 'rb') as f:
		all_data = f.readlines()

		with open(output_file, 'a+') as new_f:
			for i in all_data:
				hex_str = binascii.b2a_hex(i).decode('unicode_escape')
				new_f.write(str(hex_str) + '\n')

def main():
	parser = argparse.ArgumentParser()
	parser.add_argument("-b", "--bin", help="bin file to be dumpped")
	parser.add_argument("-o", "--output", help="output file")
	args = parser.parse_args()
	if (not args.bin) or (not args.output):
		print(f"usage: dump_bin.py -b bin_file -o output_file")
		exit(0)

	dump(args.bin, args.output)

if __name__ == "__main__":
	main()


