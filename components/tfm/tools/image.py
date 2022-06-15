#!/usr/bin/env python3

import json
import re
import os
import argparse
import struct

s_base_addr = 0
SZ_16M = 0x1000000
TOOL_NAME = 'image.py'

def crc16(data):
	crc = 0xFFFF
	for pos in data:
		crc ^= pos
		for i in range(8):
			if ((crc & 1) != 0):
				crc >>= 1
				crc ^= 0xA001
			else:
				crc >>= 1
	crc_int = (((crc & 0xff) << 8) + (crc >> 8))
	crc_hex = '{:0>4x}'.format(crc_int)
	crc_bin = bytes(crc_hex, 'utf-8')
	print(f'crc_bin={crc_bin}')
	return crc_bin

# Physical address to virtual address
def p2v(addr):
	return (addr - s_base_addr)

def hex2int(str):
	return int(str, base=16) % (2**32)

def decimal2int(str):
	return int(str, base=10) % (2**32)

def crc_size(size):
	return ((size >> 5) * 34)

def crc_addr(addr):
	return crc_size(addr)

def size2int(str):
	size_str = re.findall(r"\d+", str)
	size = decimal2int(size_str[0])

	unit = re.findall(r"[k|K|m|M|g|G|b|B]+", str)
	if (unit[0] == 'b') or (unit[0] == 'B'):
		return size
	if (unit[0] == 'k') or (unit[0] == 'K'):
		return size * (1<<10)
	elif (unit[0] == 'm') or (unit[0] == 'M'):
		return size * (1<<20)
	elif (unit[0] == 'g') or (unit[0] == 'G'):
		return size * (1<<30)
	else:
		print(f'invalid size unit {unit[0]}, must be "k/K/m/M/g/G"')

def is_out_of_range(addr, size):
	if ( (addr + size) >= SZ_16M):
		return True
	return False

class image:
	def check_field(self):
		pass

	def __init__(self, idx, img_dic, base_addr):
		global s_base_addr
		self.idx = idx
		self.img_json = img_dic

		self.check_field()

		self.fill_en = False
		self.firmware_en = False
		self.value_en = False
		if "firmware" in self.img_json.keys():
			self.firmware_en = True
			self.firmware = img_dic['firmware']
			self.firmware_size = os.path.getsize(self.firmware)
			self.crc_firmware_size = crc_size(self.firmware_size)
			if not os.path.exists(self.firmware):
				print(f'image{idx} firmware %s not exists' %(self.firmware))
				exit(0)
		elif "value" in self.img_json.keys():
			self.value_en = True
			self.value = img_dic['value']
		elif "fill" in self.img_json.keys():
			self.fill_en = True
			self.fill = img_dic['fill']
		else:
			print(f'Shouls pecify "firmware", "value" or "fill"')
			exit(0)

		self.hex_en = False
		if "hex" in self.img_json.keys():
			self.hex_en = True

		if "partition" in self.img_json.keys():
			self.partition = img_dic['partition']

		if "start_addr" in self.img_json.keys():
			self.cpu_start_addr = hex2int(img_dic['start_addr'])
		else:
			self.cpu_start_addr = base_addr
		self.cpu_size = size2int(img_dic['size'])

		# Must init s_base_addr before checking address!
		if (idx == 0):
			s_base_addr = self.cpu_start_addr
			self.cpu_start_addr = 0
		else:
			if (self.cpu_start_addr <= s_base_addr):
				print(f'image{self.idx} start_addr=%x < base_addr=%x' %(self.cpu_start_addr, s_base_addr))
				exit(0)
			self.cpu_start_addr = p2v(self.cpu_start_addr)

		if is_out_of_range(self.cpu_start_addr, self.cpu_size):
			print(f'image{self.idx} start=%x size=%x is out of range' %(self.cpu_start_addr, self.cpu_size))
			exit(0)

		#if ((self.cpu_start_addr % 32) != 0):
			#print(f'image%x start_addr=%x is not 32 bytes aligned' %(self.cpu_start_addr))
			#exit(0)

		if (self.firmware_en == True) and (self.firmware_size > self.cpu_size):
			print(f'image{idx} firmware size %x > %x' %(self.firmware_size, self.cpu_size))
			exit(0)
		self.crc_start_addr = self.cpu_start_addr
		self.crc_size = self.cpu_size
		self.crc_en = False
		self.enc_start_addr = self.cpu_start_addr
		self.enc_size = self.cpu_size
		self.enc_en = False

		if ("crc" in img_dic):
			if (img_dic['crc'] == 'y') or (img_dic['crc'] == 'Y'):
				self.crc_start_addr = crc_addr(self.cpu_start_addr)
				self.crc_size = crc_size(self.cpu_size)
				self.crc_en = True

		if is_out_of_range(self.crc_start_addr, self.crc_size):
			print(f'image{self.idx} crc is out of range')
			exit(0)

		#print(f'image%x cpu_start=%x size=%x, crc_start=%x, size=%x, enc_start=%x enc_end=%x'
		#		%(self.idx, self.cpu_start_addr, self.cpu_size, self.crc_start_addr, self.crc_size, self.enc_start_addr, self.enc_size))

	def add_crc(self):
		if (self.firmware_en):
			self.raw_buf = bytearray(self.firmware_size)
			self.crc_buf = bytearray(self.crc_firmware_size)
			with open(self.firmware, 'rb') as f:
				print(f'TODO add CRC16')
				self.crc_buf =  f.read()
				#print(f'=================> len=%u' %(len(self.raw_buf)))
		elif (self.value_en):
			if self.hex_en == True:
				self.crc_buf = bytes.fromhex(self.value)
			else:
				self.crc_buf = bytearray(self.value, encoding='utf-8')
			fill_value = 0
			pad_len = self.crc_size - len(self.value)
			for i in range(pad_len):
				self.crc_buf.append(fill_value)
		else:
			fill_value = int(self.fill)
			self.crc_buf = bytearray()
			for i in range(self.crc_size):
				self.crc_buf.append(fill_value)

class images:

	def __init__(self, args):
		if (not args.json) or (not args.outfile):
			print('usage: {TOOL_NAME} -j json_file -o outfile')
			exit(1)

		self.imgs = []
		self.output_file_name = args.outfile
		self.json_file_name = args.json 
		if not os.path.exists(args.json):
			print(f'JSON configuration file {args.json} not exists')
			exit(0)

		with open(args.json, 'r') as self.json_file:
			self.json_data = json.load(self.json_file)
		self.check_json_data()

	def check_json_data(self):
		if ("images" not in self.json_data):
			print('json does not contain field "images"!')
			exit(0)

		self.imgs_cnt = len(self.json_data['images'])
		if (self.imgs_cnt == 0):
				print(f'images of json does not contain any item!')
				exit(0)

		pre_field_addr = 0
		for idx in range(self.imgs_cnt):
			img = image(idx, self.json_data['images'][idx], pre_field_addr)
			pre_field_addr = img.crc_start_addr + img.crc_size
			self.imgs.append(img)

		for idx in range(self.imgs_cnt):
			if (idx == 0):
				continue

			pre_crc_start_addr = self.imgs[idx - 1].crc_start_addr
			pre_crc_size = self.imgs[idx - 1].crc_size
			crc_start_addr = self.imgs[idx].crc_start_addr
			if ( (pre_crc_start_addr + pre_crc_size) > crc_start_addr ):
				print(f'image%x start=%x size=%x overlapped with image%x start=%x'
						%(idx-1, pre_crc_start_addr, pre_crc_size, idx, crc_start_addr))
				exit(0)
			#check_addr(self.imgs[idx - 1], self.imgs[idx])

	def test(self):
		data = 1
		content = data.to_bytes(1, "big")
		f = open('t.bin', "wb+")
		for i in range(4096):
			f.write(content)

		f.seek(0x10240)
		f.write(content)
		f.flush()
		f.close()

	def merge_image(self):
		f = open(self.output_file_name, 'wb+')
		for idx in range(self.imgs_cnt):
			img = self.imgs[idx]
			img.add_crc()
			#print(f'merge image{idx} start=%x' %(img.crc_start_addr))
			f.seek(img.crc_start_addr)
			f.write(img.crc_buf)

		f.flush()
		f.close()

def create(args):
	if (not args.size) or (not args.outfile):
		print(f'usage: {TOOL_NAME} create -o outfile -s size [-f fill]')
		exit(1)

	fill_value = 0
	if args.fill:
		fill_value = int(args.fill)

	with open(args.outfile, 'wb+') as f:
		for i in range(int(args.size)):
			a = struct.pack('B', fill_value)
			f.write(a)

def revert(args):
	if (not args.infile) or (not args.outfile):
		print(f'usage: {TOOL_NAME} revert -i infile -o outfile')
		exit(1)

	bin_size = os.path.getsize(args.infile)
	if (bin_size % 4) != 0:
		print(f'{args.infile} is not 4 bytes aligned')
		exit(0)
	loop_cnt = bin_size >> 2
	addr = 0x0
	with open(args.outfile, 'wt+') as of:
		with open(args.infile, 'rb') as f:
			for i in range(loop_cnt):
				bin_buf = f.read(4)
				hex_buf = bin_buf.hex()
				hex_int = int(hex_buf, base=16)

				byte0 = (hex_int>>24) & 0xFF
				byte1 = (hex_int>>16) & 0xFF
				byte2 = (hex_int>>8) & 0xFF
				byte3 = (hex_int>>0) & 0xFF
				hex_int_le = (byte3 << 24) + (byte2 << 16) + (byte1 << 8) + byte0

				hex_int_revert = 0xFFFFFFFF - hex_int_le
				hex_str = str(hex_int_revert)
				revert_buf = ''.join(['@{:0>8x} '.format(addr), '%08x' %(hex_int_revert), '\n'])
				#print(f'{hex_buf} {revert_buf}')
				of.write(revert_buf)
				addr += 1
				revert_buf = ''.join(['@{:0>8x} '.format(addr), '%08x' %(hex_int_revert), '\n'])
				of.write(revert_buf)
				addr += 1

def add_crc16(args):
	if (not args.infile) or (not args.outfile):
		print(f'usage: {TOOL_NAME} -i infile -o outfile')
		exit(0)

	if not os.path.exists(args.infile):
		print(f'infile {args.infile} not exists')
		exit(0)
	
	in_file_size = os.path.getsize(args.infile)
	num_of_crc = in_file_size >> 5
	last_block_len = in_file_size % 32
	with open(args.outfile, 'wb+') as of:
		with open(args.infile, 'rb') as f:
			for i in range(num_of_crc):
				raw = f.read(32)
				crc = crc16(raw)
				of.write(raw)
				of.write(crc)
			if (last_block_len != 0):
				pad_buf = []
				raw = f.read(last_block_len)
				pad_len = 32 - last_block_len
				for c in raw:
					pad_buf.append(c)
				for i in range(pad_len):
					pad_buf.append(0xff)
				crc = crc16(pad_buf)
				pad_buf_bytes = bytes(pad_buf)
				of.write(pad_buf_bytes)
				of.write(crc)

def asii(args):
	if (not args.infile) or (not args.outfile):
		print(f'usage: {TOOL_NAME} -i infile -o outfile')
		exit(0)

	infile_len = os.path.getsize(args.infile)
	with open(args.outfile, 'wt+') as of:
		with open(args.infile, 'rb') as f:
			for i in range(infile_len):
				a_byte = f.read(1)
				a_byte_hex = a_byte.hex()
				a_byte_int = int(a_byte_hex, base=16)
				a_byte_line = ''.join(['%02x' %(a_byte_int), '\n'])
				of.write(a_byte_line)

def dump(args):
	if (not args.infile) or (not args.outfile):
		print(f'usage: {TOOL_NAME} -i infile -o outfile')
		exit(0)

	with open(args.outfile, "wt+") as of:
		with open(args.infile, "rb") as f:
			file_size = os.path.getsize(args.infile)
			for i in range(file_size):
				byte = f.read(1)
				hex_byte = byte.hex()
				of.write(hex_byte)

def parse_args():
	parser = argparse.ArgumentParser("Beken image tool.")
	parser.add_argument("cmd",
			type=str, choices=['create', 'crc16', 'merge', 'revert', 'modify', 'encrypt', 'asii', 'dump'],
			help="image commands")

	parser.add_argument("-i", "--infile", help="input file name")
	parser.add_argument("-o", "--outfile", help="output file name")
	parser.add_argument("-j", "--json", help="json configuration")
	parser.add_argument("-s", "--size", help="bin size")
	parser.add_argument("-f", "--fill", help="fill value")

	args = parser.parse_args()

	if args.cmd == 'create':
		create(args)
	elif args.cmd == 'crc16':
		add_crc16(args)
	elif args.cmd == 'merge':
		img = images(args)
		img.merge_image()
	elif args.cmd == 'revert':
		revert(args)
	elif args.cmd == 'asii':
		asii(args)
	elif args.cmd == 'dump':
		dump(args)
	else:
		print(f'unknow command')
		exit(0)

def main():
	parse_args()

if __name__ == "__main__":
	main()
