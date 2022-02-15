#ifndef _BK_TIMER_H_
#define _BK_TIMER_H_

#if (CONFIG_SOC_BK7271)

#define TIMER_CHANNEL_NO                               6

#define BK_TIMER_FAILURE							(1)
#define BK_TIMER_SUCCESS							(0)

#define TIMER_GROUNP_BASE(x)                         (0x00802780 +0x40 *(x))

#define TIMER0_CNT(x)                                (TIMER_GROUNP_BASE(x)  + 0 * 4)
#define TIMER1_CNT(x)                                (TIMER_GROUNP_BASE(x)  + 1 * 4)
#define TIMER2_CNT(x)                                (TIMER_GROUNP_BASE(x)  + 2 * 4)

#define TIMER0_2_CTL(x)								(TIMER_GROUNP_BASE(x)  + 3 * 4)

#define TIMERCTL0_EN_SET                               (0x01UL << 0)
#define TIMERCTL1_EN_SET                               (0x01UL << 1)
#define TIMERCTL2_EN_SET                               (0x01UL << 2)

#define TIMERCTLA_CLKDIV_POSI                          (3)
#define TIMERCTLA_CLKDIV_MASK                          (0x07)

#define TIMERCTLB_CLKDIV_POSI                          (3)
#define TIMERCTLB_CLKDIV_MASK                          (0x07)

#define TIMERCTLA_INT_POSI                             (7)
#define TIMERCTLB_INT_POSI                             (7)

#define TIMERCTL0_INT_MASK                             (0x01UL << 7)
#define TIMERCTL1_INT_MASK                             (0x01UL << 8)
#define TIMERCTL2_INT_MASK                             (0x01UL << 9)
#define TIMER0_3_INT_MASK                              (0x07UL << 7)


#define TIMER0_2_READ_CTRL(x)						 (TIMER_GROUNP_BASE(x)  + 4* 4)

#define TIMER_READ_CNT_ENABLE							(1 << 0)
#define TIMER_READ_CNT_MASK								(1 << 0)

#define TIMER_READ_CNT_CHANNEL(x)					   ((x) << 2)
#define TIMER0_2_READ_CNT_MASK						   (3 << 2)

#define TIMER0_2_READ_VALUE(x)						 (TIMER_GROUNP_BASE(x)  + 5* 4)

#define REG_TIMERCTLA_PERIOD_ADDR(n)                 (TIMER_GROUNP_BASE(0) +  0x04 * (n))
#define REG_TIMERCTLB_PERIOD_ADDR(n)                 (TIMER_GROUNP_BASE(1) + 0x04 * ((n) - 3))


UINT32 bk_timer_ctrl(UINT32 cmd, void *param);
#endif
#endif //_TIMER_H_



