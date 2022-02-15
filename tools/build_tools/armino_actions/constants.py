import collections
import multiprocessing
import os
import platform

# Make flavors, across the various kinds of Windows environments & POSIX...
if 'MSYSTEM' in os.environ:  # MSYS
    MAKE_CMD = 'make'
    MAKE_GENERATOR = 'MSYS Makefiles'
elif os.name == 'nt':  # other Windows
    MAKE_CMD = 'mingw32-make'
    MAKE_GENERATOR = 'MinGW Makefiles'
elif platform.system() == 'FreeBSD':
    MAKE_CMD = 'gmake'
    MAKE_GENERATOR = 'Unix Makefiles'
else:
    MAKE_CMD = 'make'
    MAKE_GENERATOR = 'Unix Makefiles'

GENERATORS = collections.OrderedDict([
    # - command: build command line
    # - version: version command line
    # - dry_run: command to run in dry run mode
    # - verbose_flag: verbose flag
    ('Ninja', {
        'command': ['ninja'],
        'version': ['ninja', '--version'],
        'dry_run': ['ninja', '-n'],
        'verbose_flag': '-v'
    }),
    (MAKE_GENERATOR, {
        'command': [MAKE_CMD, '-j', str(multiprocessing.cpu_count() + 2)],
        'version': [MAKE_CMD, '--version'],
        'dry_run': [MAKE_CMD, '-n'],
        'verbose_flag': 'VERBOSE=1',
    })
])

SUPPORTED_TARGETS = ['bk7231n', 'bk7231u', 'bk7251', 'bk7271', 'bk7236', 'bk7256', 'bk7256_cp1']

PREVIEW_TARGETS = ['']
