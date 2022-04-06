/*
 * Copyright (c) 2012-2018 Andes Technology Corporation
 * All rights reserved.
 *
 */
#include"core_v5.h"

extern void c_startup(void);
extern void system_init(void);
extern void __libc_init_array(void);
extern void boot_main(void);

void reset_handler(void)
{
    write_csr(NDS_MCACHE_CTL, (read_csr(NDS_MCACHE_CTL) | 0x1)); // Enable  ICache
	write_csr(NDS_MCACHE_CTL, (read_csr(NDS_MCACHE_CTL) | 0x2)); // Enable  DCache
    *((volatile unsigned long *) (0x44000600)) = 0x5A0000;
    *((volatile unsigned long *) (0x44000600)) = 0xA50000;
    *((volatile unsigned long *) (0x44800000)) = 0x5A0000;
    *((volatile unsigned long *) (0x44800000)) = 0xA50000;

	/*
	 * Initialize LMA/VMA sections.
	 * Relocation for any sections that need to be copied from LMA to VMA.
	 */
	c_startup();

	/* Call platform specific hardware initialization */
	system_init();

	/* Do global constructors */
	__libc_init_array();

	/* Entry function */
	boot_main();
}
