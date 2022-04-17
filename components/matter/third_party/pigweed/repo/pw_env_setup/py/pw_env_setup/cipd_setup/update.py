#!/usr/bin/env python
# Copyright 2020 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
"""Installs or updates prebuilt tools.

Must be tested with Python 2 and Python 3.

The stdout of this script is meant to be executed by the invoking shell.
"""

from __future__ import print_function

import argparse
import json
import os
import platform
import re
import subprocess
import sys


def parse(argv=None):
    """Parse arguments."""

    script_root = os.path.join(os.environ['PW_ROOT'], 'pw_env_setup', 'py',
                               'pw_env_setup', 'cipd_setup')
    git_root = subprocess.check_output(
        ('git', 'rev-parse', '--show-toplevel'),
        cwd=script_root,
    ).decode('utf-8').strip()

    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        '--install-dir',
        dest='root_install_dir',
        default=os.path.join(git_root, '.cipd'),
    )
    parser.add_argument('--package-file',
                        dest='package_files',
                        metavar='PACKAGE_FILE',
                        action='append')
    parser.add_argument('--cipd',
                        default=os.path.join(script_root, 'wrapper.py'))
    parser.add_argument('--cache-dir',
                        default=os.environ.get(
                            'CIPD_CACHE_DIR',
                            os.path.expanduser('~/.cipd-cache-dir')))

    return parser.parse_args(argv)


def check_auth(cipd, package_files, spin):
    """Check have access to CIPD pigweed directory."""

    paths = []
    for package_file in package_files:
        with open(package_file, 'r') as ins:
            # This is an expensive RPC, so only check the first few entries
            # in each file.
            for i, entry in enumerate(json.load(ins).get('packages', ())):
                if i >= 3:
                    break
                parts = entry['path'].split('/')
                while '${' in parts[-1]:
                    parts.pop(-1)
                paths.append('/'.join(parts))

    username = None
    try:
        output = subprocess.check_output([cipd, 'auth-info'],
                                         stderr=subprocess.STDOUT).decode()
        logged_in = True

        match = re.search(r'Logged in as (\S*)\.', output)
        if match:
            username = match.group(1)

    except subprocess.CalledProcessError:
        logged_in = False

    def _check_all_paths():
        inaccessible_paths = []

        for path in paths:
            # Not catching CalledProcessError because 'cipd ls' seems to never
            # return an error code unless it can't reach the CIPD server.
            output = subprocess.check_output(
                [cipd, 'ls', path], stderr=subprocess.STDOUT).decode()
            if 'No matching packages' not in output:
                continue

            # 'cipd ls' only lists sub-packages but ignores any packages at the
            # given path. 'cipd instances' will give versions of that package.
            # 'cipd instances' does use an error code if there's no such package
            # or that package is inaccessible.
            try:
                subprocess.check_output([cipd, 'instances', path],
                                        stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError:
                inaccessible_paths.append(path)

        return inaccessible_paths

    inaccessible_paths = _check_all_paths()

    if inaccessible_paths and not logged_in:
        with spin.pause():
            stderr = lambda *args: print(*args, file=sys.stderr)
            stderr()
            stderr('Not logged in to CIPD and no anonymous access to the '
                   'following CIPD paths:')
            for path in inaccessible_paths:
                stderr('  {}'.format(path))
            stderr()
            stderr('Attempting CIPD login')
            try:
                subprocess.check_call([cipd, 'auth-login'])
            except subprocess.CalledProcessError:
                stderr('CIPD login failed')
                return False

        inaccessible_paths = _check_all_paths()

    if inaccessible_paths:
        stderr = lambda *args: print(*args, file=sys.stderr)
        stderr('=' * 60)
        username_part = ''
        if username:
            username_part = '({}) '.format(username)
        stderr('Your account {}does not have access to the following '
               'paths'.format(username_part))
        for path in inaccessible_paths:
            stderr('  {}'.format(path))
        stderr('=' * 60)
        return False

    return True


def _platform():
    osname = {
        'darwin': 'mac',
        'linux': 'linux',
        'windows': 'windows',
    }[platform.system().lower()]

    if platform.machine().startswith(('aarch64', 'armv8')):
        arch = 'arm64'
    elif platform.machine() == 'x86_64':
        arch = 'amd64'
    elif platform.machine() == 'i686':
        arch = 'i386'
    else:
        arch = platform.machine()

    return '{}-{}'.format(osname, arch).lower()


def all_package_files(env_vars, package_files):
    """Recursively retrieve all package files."""

    result = []
    to_process = []
    for pkg_file in package_files:
        args = []
        if env_vars:
            args.append(env_vars.get('PW_PROJECT_ROOT'))
        args.append(pkg_file)

        # The signature here is os.path.join(a, *p). Pylint doesn't like when
        # we call os.path.join(*args), but is happy if we instead call
        # os.path.join(args[0], *args[1:]). Disabling the option on this line
        # seems to be a less confusing choice.
        path = os.path.join(*args)  # pylint: disable=no-value-for-parameter

        to_process.append(path)

    while to_process:
        package_file = to_process.pop(0)
        result.append(package_file)

        with open(package_file, 'r') as ins:
            entries = json.load(ins).get('included_files', ())

        for entry in entries:
            entry = os.path.join(os.path.dirname(package_file), entry)

            if entry not in result and entry not in to_process:
                to_process.append(entry)

    return result


def write_ensure_file(package_file, ensure_file):
    with open(package_file, 'r') as ins:
        packages = json.load(ins).get('packages', ())

    with open(ensure_file, 'w') as outs:
        outs.write('$VerifiedPlatform linux-amd64\n'
                   '$VerifiedPlatform mac-amd64\n'
                   '$ParanoidMode CheckPresence\n')

        for pkg in packages:
            # If this is a new-style package manifest platform handling must
            # be done here instead of by the cipd executable.
            if 'platforms' in pkg and _platform() not in pkg['platforms']:
                continue

            outs.write('@Subdir {}\n'.format(pkg.get('subdir', '')))
            outs.write('{} {}\n'.format(pkg['path'], ' '.join(pkg['tags'])))


def package_installation_path(root_install_dir, package_file):
    """Returns the package installation path.

    Args:
      root_install_dir: The CIPD installation directory.
      package_file: The path to the .json package definition file.
    """
    return os.path.join(root_install_dir,
                        os.path.basename(os.path.splitext(package_file)[0]))


def update(
    cipd,
    package_files,
    root_install_dir,
    cache_dir,
    env_vars=None,
    spin=None,
):
    """Grab the tools listed in ensure_files."""

    package_files = all_package_files(env_vars, package_files)

    if not check_auth(cipd, package_files, spin):
        return False

    # TODO(mohrr) use os.makedirs(..., exist_ok=True).
    if not os.path.isdir(root_install_dir):
        os.makedirs(root_install_dir)

    if env_vars:
        env_vars.prepend('PATH', root_install_dir)
        env_vars.set('PW_CIPD_INSTALL_DIR', root_install_dir)
        env_vars.set('CIPD_CACHE_DIR', cache_dir)

    pw_root = None
    if env_vars:
        pw_root = env_vars.get('PW_ROOT', None)
    if not pw_root:
        pw_root = os.environ['PW_ROOT']

    # Run cipd for each json file.
    for package_file in package_files:
        if os.path.splitext(package_file)[1] == '.ensure':
            ensure_file = package_file
        else:
            ensure_file = os.path.join(
                root_install_dir,
                os.path.basename(
                    os.path.splitext(package_file)[0] + '.ensure'))
            write_ensure_file(package_file, ensure_file)

        install_dir = package_installation_path(root_install_dir, package_file)

        name = os.path.basename(install_dir)

        cmd = [
            cipd,
            'ensure',
            '-ensure-file', ensure_file,
            '-root', install_dir,
            '-log-level', 'debug',
            '-json-output',
            os.path.join(root_install_dir, '{}-output.json'.format(name)),
            '-cache-dir', cache_dir,
            '-max-threads', '0',  # 0 means use CPU count.
        ]  # yapf: disable

        # TODO(pwbug/135) Use function from common utility module.
        log = os.path.join(root_install_dir, '{}.log'.format(name))
        try:
            with open(log, 'w') as outs:
                print(*cmd, file=outs)
                subprocess.check_call(cmd,
                                      stdout=outs,
                                      stderr=subprocess.STDOUT)
        except subprocess.CalledProcessError:
            with open(log, 'r') as ins:
                sys.stderr.write(ins.read())
                raise

        # Set environment variables so tools can later find things under, for
        # example, 'share'.
        if env_vars:
            # Some executables get installed at top-level and some get
            # installed under 'bin'.
            env_vars.prepend('PATH', install_dir)
            env_vars.prepend('PATH', os.path.join(install_dir, 'bin'))
            env_vars.set('PW_{}_CIPD_INSTALL_DIR'.format(name.upper()),
                         install_dir)

            # Windows has its own special toolchain.
            if os.name == 'nt':
                env_vars.prepend('PATH',
                                 os.path.join(install_dir, 'mingw64', 'bin'))

    return True


if __name__ == '__main__':
    update(**vars(parse()))
    sys.exit(0)
