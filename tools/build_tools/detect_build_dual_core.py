#!/usr/bin/env python3

import os
import subprocess
import sys

dual_core_porjects = [
        ['bk7256', 'legacy_app'],
        ['bk7256', 'bk7256xx_mp'],
        ['bk7235', 'bk7235_config2'],
        ['bk7237', 'legacy_app']
    ]

## argv[1] = $(ARMINO_SOC)
## argv[2] = $(PROJECT)
def main(argv):
    if (len(argv) > 2):
        for dual_core_porject in dual_core_porjects:
            if(argv[1] == dual_core_porject[0] and argv[2] == dual_core_porject[1]):
                print('1')
                return

    print('0')
    return


if __name__ == "__main__":
    main(sys.argv)