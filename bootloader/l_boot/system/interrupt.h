/*************************************************************
 * @file        interrupt.h
 * @brief       Header file of interrupt.c
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */

#ifndef __DRIVER_INTRRUPT_H__

#define __DRIVER_INTRRUPT_H__


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


    // define the IRQ handler attribute for this compiler
#define __IRQ                      __irq

    // define the FIQ handler attribute for this compiler
#define __FIQ                      __irq


    void int_init(void);
    void int_enable(int index);
    void int_disable(int index);

    // IRQ handler
    __IRQ void int_irq(void);

    // FIQ handler
    __FIQ void int_fiq(void);


#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif  /* __DRIVER_INTRRUPT_H__ */

