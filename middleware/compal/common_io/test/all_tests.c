#include "unity_fixture.h"
#include <common/bk_include.h>


static void run_all_tests(void)
{
    //RUN_TEST_GROUP(TEST_IOT_UART);
    //RUN_TEST_GROUP(TEST_IOT_ADC);
    //RUN_TEST_GROUP(TEST_IOT_SPI);
    
    // RUN_TEST_GROUP(TEST_IOT_I2C);
#if(CONFIG_PWM)
    RUN_TEST_GROUP(TEST_IOT_PWM);
#endif
    RUN_TEST_GROUP(TEST_IOT_GPIO);
}

int common_io_test_main(int argc, const char * argv[])
{
    return UnityMain(argc, argv, run_all_tests);
}
