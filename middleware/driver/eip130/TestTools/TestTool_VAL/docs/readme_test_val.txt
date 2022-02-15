+======================================================================+
| Copyright (c) 2014-2018 INSIDE Secure B.V. All Rights Reserved.      |
|                                                                      |
| Subject   : README for the test Tool for the VAL API                 |
| Product   : EIP-13x Driver Development Kit                           |
| Date      : 08 November, 2018                                        |
|                                                                      |
+======================================================================+

ATTENTION: This program can altered the OTP, which can make the SOC
           unusable for future use. So please use this program wisely.

This is the readme file for building the Test Tool for validating the
of VAL API. The Test Tool tests all functions provided by the VAL API.
It is built both for kernel deployment (the Test Tool will run as a Linux
kernel module using the kernel mode driver) and for user mode deployment
(the Test Tool will run as a user mode program and it is linked with the
user mode driver or kernel mode driver depending on the deployment).

The Test Tool has all test vectors hardcoded in the C source files as
initialized structures. These files are located in the test_vectors
subdirectory and are compiled into the program. Test vectors typically
include the applicable key information, input and expected output.
Whenever the program is started, it runs all tests and it reports
whether each test passed or failed.

The Test Tool executes one Test Suite that has multiple Test Cases. Each
Test Cases will test a specific part of the VAL API and consists of one or
more Known Answer Tests (KAT). A test can be skipped based on the result
environment and hardware probing. For example, if the Test Tool is run in
non-secure environment certain operations not be executed (e.g. will
generate an error) like TRNG reseed.

When the Demo Application is used in user mode, the application provides
additional functionality that can be selected via command line arguments.
Please use the argument '-help' to retrieve information about this additional
functionality.


Build Instructions:
--------------------

 - Ensure that the ARCH, KERNEL_DIR and optionally CROSS_COMPILE environment
   variables are set as described in Examples/TestTool_VAL/build/Makefile

 - Build the Driver that the Test Tool depends on, otherwise
   the build cannot start. The Test Tool will use the Module.symver
   file from the Driver.

 - Go to the TestTool_VAL build directory
   # cd Examples/TestTool_VAL/build

 - Build the test_val module and user mode application.
   # make

   This will build test_val_k.ko, test_val_combined_up and test_val_combined_u.


Usage Instructions:
-------------------

  - Make sure the Driver and Test Tool modules are built.

  - Run the script
    # Examples/TestTool_VAL/scripts/run_val.sh

    This runs the script in kernel mode. A successful run of the test
    should show "PASSED" in the output for each test executed.

  - Make sure the UMPCI driver is loaded, and the /dev/umpci_c device node
    created (check the /proc/devices file for the major number, 254 in
    the example below). Then run the program test_val_combined_u. This
    should produce the same output.

    # insmod umpci_k.ko
    # mknod /dev/umcpi_c c 254 0
    # ./test_val_combined_u


<end-of-document>
