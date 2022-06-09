# -*- coding: utf-8 -*-
#
# English Language RTD & Sphinx config file
#
# Uses ../conf_common.py for most non-language-specific settings.

# Importing conf_common adds all the non-language-specific
# parts to this conf module
import sys
import os
sys.path.insert(0, os.path.abspath('..'))
from conf_common import *  # noqa: F401, F403 - need to make available everything from common
from local_util import download_file_if_missing  # noqa: E402 - need to import from common folder

import datetime

build_year = datetime.datetime.now().year
build_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

# General information about the project.
project = u'博通集成 ARMINO 开发框架'
copyright = u'2020 - {} 博通集成电路（上海）股份有限公司。 更新时间: {} '.format(build_year, build_time)

# The language for content autogenerated by Sphinx. Refer to documentation
# for a list of supported languages.
language = 'zh_CN'

# Set up font for blockdiag, nwdiag, rackdiag and packetdiag
blockdiag_fontpath = '../_static/DejaVuSans.ttf'
seqdiag_fontpath = '../_static/DejaVuSans.ttf'
actdiag_fontpath = '../_static/DejaVuSans.ttf'
nwdiag_fontpath = '../_static/DejaVuSans.ttf'
rackdiag_fontpath = '../_static/DejaVuSans.ttf'
packetdiag_fontpath = '../_static/DejaVuSans.ttf'
