+======================================================================+
| Copyright (c) 2016-2018 INSIDE Secure B.V. All Rights Reserved.      |
|                                                                      |
| Subject   : README for the Demo Application for special operations   |
|             provided by the VAL API                                  |
| Product   : EIP-13x Driver Development Kit                           |
| Date      : 08 November, 2018                                        |
|                                                                      |
+======================================================================+

ATTENTION: This program can altered the OTP, which can make the SOC
           unusable for future use. So please use this program wisely.

This is the readme file for building the Demo Application for special
operations that are provided by the VAL API and must be used in
combination with the EIP-13x driver. The Demo Application shows the usage
of the VAL API for these special operations which can make hardware
unusable for normal use.
The Demo Application is built for user mode deployment only.

The usage of the Demo Application can be retrieved when the program is
executed with the command line argument '-help'. Based on the given
arguments, the Demo Application can perform the following actions/tests:

- Reset firmware (hardware).
- Configure and start the TRNG.
- Initialization of the OTP with a default COID and HUK.
- Initialization of the OTP with a default COID and generated random HUK.
- Increment the 96-bit monotonic counter which is located in OTP.
- Zeroize/Destroy the OTP.
- Write a default eMMC authentication key to OTP.
- Check the eMMC read and write functions based on the default eMMC
  authentication key located in OTP.
- Write a default Milenage K and OPc values to OTP.
- Check Milenage AUTN verification related functionality.
- Check Milenage AUTS generation related functionality.
- Set hardware in Sleep mode.
- Resume hardware from Sleep mode.
- Set hardware in Hibernation mode.
- Resume hardware from hibernation mode.
- Read the HW registers for Module Status, HW options and HW version.
- Read system information
- Perform self-test (switch to FIPS mode)


Build Instructions:
--------------------

 - Ensure that the ARCH, KERNEL_DIR and optionally CROSS_COMPILE environment
   variables are set as described in Examples/DemoApp_VAL_Specials/build/Makefile

 - Build the required libraries on which the application variants that are
   built depend.

 - Go to the DemoApp_VAL_Specials build directory
   # cd Examples/DemoApp_VAL_Specials/build

 - Build the da_val_specials user mode application.
   # make

   This will build da_val_specials_up, da_val_specials_combined_u and
   da_val_specials_combined_up.


Usage Instructions:
-------------------

  - Make sure the Global Control and VAL driver are loaded, and the /dev/vexp_c
    device node created. Then run the program da_val_specials_up. Use the
    following command sequence example to run the program:

    # insmod driver_gc_k.ko
    # insmod driver_val_k.ko
    # major_nr=`awk "\\$2==\"vexp_c\" {print \\$1}" /proc/devices`
    # mknod /dev/vexp_c c ${major_nr} 0
    # ./da_val_specials_up [<arguments> ...]
    # rmmod driver_val_k
    # rmmod driver_gc_k
    # rm /dev/vexp_c

  - Make sure the UMPCI driver is loaded, and the /dev/umpci_c device node
    created. Then run the program da_val_specials_combined_u. Use the
    following command sequence example to run the program:

    # insmod umpci_k.ko
    # major_nr=`awk "\\$2==\"umcpi_c\" {print \\$1}" /proc/devices`
    # mknod /dev/umcpi_c c ${major_nr} 0
    # ./da_val_specials_combined_u [<arguments> ...]
    # rmmod umpci_k
    # rm /dev/umcpi_c


<end-of-document>

