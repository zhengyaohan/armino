#!/usr/bin/python3
#脚本执行命令为
#/usr/bin/python3 /home/longbao.hu/workspace/armino/tools/build_tools/armino_size_static.py --soc bk7231n --dirs /home/longbao.hu/workspace/armino/components/bk_libs --outfile size.txt
#/usr/bin/python3 ../tools/build_tools/armino_size_static.py --soc bk7256 --dirs ../components/bk_libs --outfile size.txt

import argparse
import sys
import re
import os

soc_type = ""
size_tools = ""
data = ""


def get_size(m):
    global data, size_tools, soc_type
    file_path = m
    cmd = size_tools+" " + file_path
    with os.popen(cmd, "r") as lines:
        text_size = 0
        code_size = 0
        rodata_size = 0
        data_size = 0
        bss_size = 0
        dec_size = 0
        hex_size = 0
        for line in lines:
            #print (line)
            data = data + line
            if ".a" in file_path:
                if "text\t" in line:
                    continue
                nums = re.findall("[\s+(\d+)\t*]\s+([0-9a-fA-F]+)\t", line, re.X)
                #print (nums)
                if soc_type == "riscv":
                    if len(nums) < 6:
                        return -1
                    text_size += int(nums[0])
                    code_size += int(nums[1])
                    rodata_size += int(nums[2])
                    data_size += int(nums[3])
                    bss_size += int(nums[4])
                    dec_size += int(nums[5])
                    hex_size += int(nums[6], 16)
                elif soc_type == "arm":
                    if len(nums) < 4:
                        return -1
                    text_size += int(nums[0])
                    data_size += int(nums[1])
                    bss_size += int(nums[2])
                    dec_size += int(nums[3])
                    hex_size += int(nums[4], 16)
        if ".a" in file_path:
            #print (text_size, code_size, rodata_size, data_size, bss_size, dec_size, hex(hex_size))
            data = data + str(text_size).rjust(7,' ')+"\t"
            if soc_type == "riscv":
                data = data + str(code_size).rjust(7,' ')+"\t"
                data = data + str(rodata_size).rjust(7,' ')+"\t"
            data = data + str(data_size).rjust(7,' ')+"\t"
            data = data + str(bss_size).rjust(7,' ')+"\t"
            data = data + str(dec_size).rjust(7,' ')+"\t"
            data = data + str(hex(hex_size)).rjust(7,' ')+"\t"
            data = data + file_path
            data = data + "\n\n"

def find_file(path):
    if (os.path.exists(path)):
        files = os.listdir(path)
        for file in files:
            m = os.path.join(path, file)
            if (os.path.isdir(m)):
                find_file(m)
            elif (file.endswith('.a')):
                #print (m)
                get_size(m)

def main():
    global data, size_tools, soc_type
    parser = argparse.ArgumentParser(description='armino_size - a tool to print size information from an BDK MAP file')
    parser.add_argument('--soc', dest='soc', default=None)
    parser.add_argument('--dirs', dest='dirs', default=None)
    parser.add_argument('--outfile', dest='outfile', default=None)
    args = parser.parse_args()
    #print (args.soc)
    #print (args.dirs)
    #print (args.outfile)
    if args.soc == None or args.dirs == None or args.outfile == None:
        print ("need more param")
        return
    switcher = {
        "bk7231n": "arm",
        "bk7231u": "arm",
        "bk7236": "arm",
        "bk7251": "arm",
        "bk7256": "riscv",
        "bk7256_cp1": "riscv-d",
        "bk7271": "arm",
    }
    soc_type = switcher.get(args.soc,"None")
    if soc_type == "arm":
        size_tools = "/opt/gcc-arm-none-eabi-5_4-2016q3/bin/arm-none-eabi-size"
    elif soc_type == "riscv":
        size_tools = "/opt/risc-v/nds32le-elf-mculib-v5/bin/riscv32-elf-size"
    elif soc_type == "riscv-d":
        size_tools = "/opt/risc-v/nds32le-elf-mculib-v5d/bin/riscv32-elf-size"
    #print (size_tools)
    if (soc_type == "None"):
        print ("Error soc")
        return
    if (os.path.exists("app.elf")):
        get_size("app.elf")
    data = data + "\n"
    find_file(os.getenv('PWD'))
    find_file(args.dirs+"/"+args.soc)
    #print (data)
    with open(args.outfile, 'w') as fw:
        fw.write(data)


if __name__ == "__main__":
    main()
