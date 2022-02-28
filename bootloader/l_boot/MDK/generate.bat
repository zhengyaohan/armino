fromelf -c  .\Objects\bk_bootloader_low.axf -o .\Objects\bkdump.txt
copy bk_bootloader_low.bin ..\..\up_boot\tools\com\bk_bootloader_low.bin
encrypt.exe bk_bootloader_low.bin 00000000
del bk_bootloader_low.out