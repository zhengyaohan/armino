Build multiple sdkconfig:

1. Build default prod: armino.py build
2. Build prod1: armino.py -B build_prod1 -D SDKCONFIG_DEFAULTS="sdkconfig.prod_common;sdkconfig.prod1" build
3. Build prod2: -B build_prod2 -D SDKCONFIG_DEFAULTS="sdkconfig.prod_common;sdkconfig.prod2" build
