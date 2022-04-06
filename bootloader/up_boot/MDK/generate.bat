fromelf -c  .\Objects\bk_bootloader_up.axf -o .\Objects\bkdump.txt
..\tools\com\link_t.exe bk_bootloader_up.bin ..\tools\com\bk_bootloader_low.bin
move bk_bootloader_up_all.bin bootloader.bin
.\rt_partition_tool_cli.exe bootloader.bin partition_default.json
encrypt.exe bk_bootloader_up.bin 00000000
encrypt.exe bootloader.bin 00000000
del bk_bootloader_up.out
del bootloader.out