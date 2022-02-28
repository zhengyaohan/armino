/* sfzutf.h
 *
 * Description: SFZUTF header.
 */

/*****************************************************************************
* Copyright (c) 2014-2018 INSIDE Secure B.V. All Rights Reserved.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef INCLUDE_GUARD_SFZUTF_H
#define INCLUDE_GUARD_SFZUTF_H

#include "basic_defs.h"
#include "log.h"
#include "clib.h"

#include "user_config.h"		////"EIP_130_DRIVER_TYPE_SEL"

#ifdef SFZUTF_USERMODE
 #include <stdlib.h>
   #if(EIP_130_DRIVER_TYPE_SEL == EIP_130_DRIVER_TYPE_BEKEN)
    #include "adapter_sleep.h"	////DelayUs
 	#define SFZUTF_USLEEP(x)   DelayUs(x)
   #else
 	#include <unistd.h>  // usleep
 	#define SFZUTF_USLEEP(x)   usleep(x)
   #endif
 #define SFZUTF_MALLOC(x)   malloc((x)>0?(x):1)
 #define SFZUTF_CALLOC(x,y) malloc(x*y)
 #define SFZUTF_FREE(x)     free(x)
#else
 #define EXPORT_SYMTAB
 
 #if(EIP_130_DRIVER_TYPE_SEL == EIP_130_DRIVER_TYPE_LINUX)
   #include <linux/init.h>
   #include <linux/module.h>
   #include <linux/kernel.h>       // printk
   #include <linux/slab.h>
   #include <asm/delay.h>          // udelay
 #endif
 
 #define SFZUTF_MALLOC(x)   kmalloc((x)>0?(x):1, GFP_KERNEL)
 #define SFZUTF_CALLOC(x,y) kmalloc(x*y, GFP_KERNEL)
 #define SFZUTF_FREE(x)     kfree(x)
 #define SFZUTF_USLEEP(x)   udelay(x)
#endif


#define SFZUTF_MEMCMP(a,b,l) memcmp(a, b, l)
#define SFZUTF_MEMCPY(a,b,l) memcpy(a, b, l)
static inline uint32_t
sfzutf_strlen(
    const char * str)
{
    uint32_t len = 0;
    const char *string_it = str;
    while(*string_it)
    {
        len++;
        string_it++;
    }
    return len;
}
#define SFZUTF_STRLEN(s) sfzutf_strlen(s)

/* Compositions of above, for common string operations. */
#define SFZUTF_STREQ(s1,s2)                             \
  (SFZUTF_STRLEN(s1) == SFZUTF_STRLEN(s2) &&            \
   SFZUTF_MEMCMP((s1), (s2), SFZUTF_STRLEN(s1)) == 0)

#define SFZUTF_STREQ_PREFIX(s1,sp)                      \
  (SFZUTF_STRLEN(s1) >= SFZUTF_STRLEN(sp) &&            \
   SFZUTF_MEMCMP((s1), (sp), SFZUTF_STRLEN(sp)) == 0)


// Test framework function prototypes and defines
typedef int (*TFun)(int _i);
typedef void (*SFun)(void);


#define START_TEST(name)                       \
  static int name(int _i)                      \
  {                                            \
    LOG_INFO("%s:L%d> START\n", __func__, _i); \
    do

#define END_TEST_SUCCES         0
#define END_TEST_FAIL           (-1)
#define END_TEST_UNSUPPORTED    (-2)
#define END_TEST_ABORT          (-1000)

#define END_TEST while(0);                               \
    if (sfzutf_unsupported_quick_process()) {            \
      LOG_CRIT("%s:L%d> NOT SUPPORTED\n", __func__, _i); \
    } else {                                             \
      LOG_CRIT("%s:L%d> PASSED\n", __func__, _i);        \
    }                                                    \
    return END_TEST_SUCCES;                              \
  }


#ifdef GCOV_PROFILE
void
__gcov_flush(); /* Function to write profiles on disk. */

#define SFZUTF_GCOV_FLUSH  __gcov_flush()
#else /* !GCOV_PROFILE */
#define SFZUTF_GCOV_FLUSH
#endif /* GCOV_PROFILE */

#define SFZUTF_FAILURE(expr,info,status)                                       \
  do {                                                                         \
    SFZUTF_GCOV_FLUSH;                                                         \
    LOG_CRIT("%s:%d> FAILED: %s%s(%d)\n",__func__,__LINE__,expr,info,status); \
    return END_TEST_FAIL;                                                      \
  } while(0)

#define SFZUTF_UNSUPPORTED(expr,info)                                      \
  do {                                                                     \
    SFZUTF_GCOV_FLUSH;                                                     \
    LOG_CRIT("%s:%d> NOT SUPPORTED: %s%s\n",__func__,__LINE__,expr,info); \
    return END_TEST_UNSUPPORTED;                                                  \
  } while(0)


/* Mark test failures */
#define fail(info,status)             \
  do {                                \
      SFZUTF_FAILURE("",info,status); \
  } while(0)

#define fail_if(expr,info,status)             \
  do {                                        \
    if (expr) {                               \
      SFZUTF_FAILURE(""#expr" ",info,status); \
    }                                         \
  } while(0)

#define fail_unless(expr,info,status)          \
  do {                                         \
    if (!(expr)) {                             \
      SFZUTF_FAILURE("!"#expr" ",info,status); \
    }                                          \
  } while(0)


/* Mark test as unsupported if condition is true. */
#define unsupported(info)        \
  do {                           \
    SFZUTF_UNSUPPORTED("",info); \
  } while(0)

#define unsupported_if(expr,info)          \
  do {                                     \
    if (expr) {                            \
      SFZUTF_UNSUPPORTED(""#expr" ",info); \
    }                                      \
  } while(0)

#define unsupported_unless(expr,info)       \
  do {                                      \
    if (!(expr)) {                          \
      SFZUTF_UNSUPPORTED("!"#expr" ",info); \
    }                                       \
  } while(0)

#define unsupported_quick(info)                                       \
  do {                                                                \
    LOG_CRIT("%s:%d> NOT SUPPORTED: %s\n", __FUNC__, __LINE_, info); \
    sfzutf_unsupported_quick();                                       \
  } while(0)


struct TestSuite
{
    struct TestSuite * NextSuite_p;
    const char * Name_p;
    struct TestCase * TestCaseList_p;
    bool Enabled;
};

struct TestCase
{
    struct TestCase * NextCase_p;
    const char * Name_p;
    SFun FixtureStartFunc;
    SFun FixtureEndFunc;
    struct Test * TestList_p;
    int TestsSuccess;
    int TestsFailed;
    bool Enabled;
};

struct Test
{
    struct Test * NextTest_p;
    const char * Name_p;
    TFun TestFunc;
    int Start;
    int NumberOfLoops;
    bool Enabled;
};


typedef enum
{
    SFZUTF_ENABLE_UNDETERMINED,
    SFZUTF_ENABLE_SINGLE,
    SFZUTF_ENABLE_AFTER,
    SFZUTF_ENABLE_ALL
}
SfzUtfEnable_t;


/* Note:
 *       Each suite needs to provide a interface that builds the test suite.
 *
 * Example:
 * int build_suite(void)
 * {
 *     struct TestSuite * TestSuite_p = sfzutf_tsuite_create("TestSuite name");
 *     if (TestSuite_p != NULL)
 *     {
 *         struct TestCase *TestCase_p = sfzutf_tcase_create(TestSuite_p, "TestCase name");
 *         if (TestCase_p != NULL)
 *         {
 *             if (sfzutf_test_add(TestCase_p, <test function>) != 0) goto FuncErrorReturn;
 *             ...
 *         }
 *         else
 *         {
 *             goto FuncErrorReturn;
 *         }
 *         TestCase_p = sfzutf_tcase_create(TestSuite_p, "Next TestCase name");
 *         if (TestCase_p != NULL)
 *         {
 *             if (sfzutf_test_add(TestCase_p, <test function>) != 0) goto FuncErrorReturn;
 *             ...
 *         }
 *         ...
 *     }
 *     return 0;
 *
 * FuncErrorReturn:
 *     return -1;
 * }
 **/

/* TestSuite helpers */
struct TestSuite *
sfzutf_tsuite_create(
        const char * Name_p);

void
sfzutf_tsuite_release(
        const char * Name_p);

struct TestSuite *
sfzutf_tsuite_get(
        const char * const SuiteName_p);

bool
sfzutf_tsuite_enable(
        struct TestSuite * TestSuite_p,
        const char * const SuiteName_p,
        const char * const TCaseName_p,
        const char * const TestName_p,
        const int * const IterValue_p,
        const SfzUtfEnable_t OrigEnableMode);

void
sfzutf_tsuite_disable(
        struct TestSuite * TestSuite_p);

int
sfzutf_tsuite_run(
        struct TestSuite * TestSuite_p);


/* TestCase helpers */
struct TestCase *
sfzutf_tcase_create(
        struct TestSuite * TestSuite_p,
        const char *Name_p);

void
sfzutf_tcase_release(
        struct TestCase *TestCaseList_p,
        const char *Name_p);

int
sfzutf_tcase_add_fixture(
        struct TestCase * TestCase_p,
        SFun StartFunc,
        SFun EndFunc);

/* Test helpers */
#define sfzutf_test_add(case,func) \
    sfzutf_ttest_create(case,func,""#func"",0)

#define sfzutf_test_add_loop(case,func,loops) \
    sfzutf_ttest_create(case,func,""#func"",loops)

int
sfzutf_ttest_create(
        struct TestCase * TestCase_p,
        TFun TestFunction,
        const char * Name_p,
        int NumberOfLoops);

void
sfzutf_ttest_release(
        struct Test * TestList_p,
        const char * Name_p);

void
sfzutf_unsupported_quick(void);

bool
sfzutf_unsupported_quick_process(void);


/* Discard const qualifier from pointer */
static inline void *
sfzutf_discard_const(
        const void * Ptr_p)
{
    union
    {
        const void * c_p;
        void * n_p;
    } Conversion;

    Conversion.c_p = Ptr_p;
    return Conversion.n_p;
}

/* Switch ordering of array.
   This implementation works byte at a time. */
static inline void
sfzutf_endian_flip(
        void *dst,
        uint32_t len)
{
    unsigned char *dst_c = dst;
    unsigned char t;
    uint32_t i;

    for(i=0; i < len/2; i++)
    {
        t = dst_c[i];
        dst_c[i] = dst_c[len-i-1];
        dst_c[len-i-1] = t;
    }
}


#endif /* Include Guard */

/* end of file sfzutf.h */
