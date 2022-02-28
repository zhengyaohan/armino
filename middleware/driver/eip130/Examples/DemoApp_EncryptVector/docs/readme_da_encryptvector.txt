+======================================================================+
| Copyright (c) 2016-2018 INSIDE Secure B.V. All Rights Reserved.      |
|                                                                      |
| Subject   : README for the Demo Application for the Encrypted Vector |
|             for PKI (EIP-154) functionality using the VAL API.       |
|             Note: This Demo Application requires a special EIP-13x   |
|                   Firmware to ensure the functionality availability. |
| Product   : EIP-13x Driver Development Kit                           |
| Date      : 08 November, 2018                                        |
|                                                                      |
+======================================================================+

This is the readme file for building the Demo Application for the
Encrypted Vector for PKI (EIP-154) functionality that is provided by
the VAL API and must be used in combination with the EIP-13x driver.
The Demo Application shows the usage of the VAL API for this special
operation which requires special EIP-13x Firmware.
The Demo Application is built for user mode deployment only.

The usage of the Demo Application can be retrieved when the program is
executed with the command line argument '-help'. Based on the given
arguments, the Demo Application performs an Asset load of Private Data,
ECDSA or RSA private key that is then encrypted to get an Encrypted Vector
that can be used in a PKI command.


Build Instructions:
--------------------

 - Ensure that the ARCH, KERNEL_DIR and optionally CROSS_COMPILE environment
   variables are set as described in Examples/DemoApp_EncryptVector/build/Makefile

 - Build the required libraries on which the application variants that are
   built depend.

 - Go to the DemoApp_EncryptVector build directory
   # cd Examples/DemoApp_EncryptVector/build

 - Build the da_encryptvector user mode application.
   # make

   This will build da_encryptvector_up, da_encryptvector_combined_u and
   da_encryptvector_combined_up.


Usage Instructions:
-------------------

  - Make sure the Global Control and VAL driver are loaded, and the /dev/vexp_c
    device node created. Then run the program da_encryptvector_up. Use the
    following command sequence example to run the program:

    # insmod driver_gc_k.ko
    # insmod driver_val_k.ko
    # major_nr=`awk "\\$2==\"vexp_c\" {print \\$1}" /proc/devices`
    # mknod /dev/vexp_c c ${major_nr} 0
    # ./da_encryptvector_up [<arguments> ...]
    # rmmod driver_val_k
    # rmmod driver_gc_k
    # rm /dev/vexp_c

  - Make sure the UMPCI driver is loaded, and the /dev/umpci_c device node
    created. Then run the program da_encryptvector_combined_u. Use the
    following command sequence example to run the program:

    # insmod umpci_k.ko
    # major_nr=`awk "\\$2==\"umcpi_c\" {print \\$1}" /proc/devices`
    # mknod /dev/umcpi_c c ${major_nr} 0
    # ./da_encryptvector_combined_u [<arguments> ...]
    # rmmod umpci_k
    # rm /dev/umcpi_c


<end-of-document>

