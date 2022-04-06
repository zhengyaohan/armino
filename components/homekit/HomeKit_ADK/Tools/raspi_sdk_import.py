#!/usr/bin/env python

from __future__ import print_function

import argparse
import os
import shutil
import subprocess
import sys
import tarfile


def build_environs(identity_file, hostname, password):
    """Build and return a map of environment variables shared across functions"""
    return {
        "ssh_opts": (
            "-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=30 -o ServerAliveInterval=10000"
            + ""
            if identity_file is None
            else " -i %s" % identity_file
        ),
        "hostname": hostname,
        "password": "" if password is None else password,
    }


def create_sdk_tarball(environs):
    """Create a tarball of Raspberry Pi development header files and libraries"""
    pexpect = subprocess.Popen("expect", shell=True, stdin=subprocess.PIPE)
    pexpect.communicate(
        """
        set timeout -1
        spawn ssh %(ssh_opts)s pi@%(hostname)s.local
        expect {
            "password: " {
                send "%(password)s\\n"
                exp_continue
            }
            "pi@%(hostname)s"
        }
        send "sudo su\\n"
        expect "root@%(hostname)s"
        send "mkdir /temp\\n"
        expect "root@%(hostname)s"
        send "tar -C / --numeric-owner --exclude=python3 --exclude=python3.7 --exclude=python2.7 --exclude=llvm-7 --exclude=valgrind --exclude=debug --exclude=./opt/vc/src/hello_pi -cf /temp/raspi-sdk.tar.bz2 lib usr/include usr/lib opt/vc usr/local/include/fdk-aac usr/local/lib/libfdk-aac.a\\n"
        expect "root@%(hostname)s"
        send "exit\\n"
        expect "pi@%(hostname)s"
        send "exit\\n"
        expect eof"""
        % environs
    )
    if pexpect.returncode != 0:
        raise RuntimeError("Tarball creation failed")


def copy_sdk_tarball(environs):
    """Copy SDK tarball file to the local computer"""
    if not os.path.exists(".tmp"):
        os.makedirs(".tmp")
    pexpect = subprocess.Popen("expect", shell=True, stdin=subprocess.PIPE)
    pexpect.communicate(
        """
        set timeout -1
        spawn scp %(ssh_opts)s pi@%(hostname)s.local:/temp/raspi-sdk.tar.bz2 .tmp/
        expect {
            "password: " {
                send "%(password)s\\n"
                exp_continue
            }
            eof
        }
        lassign [wait] pid spawnID osError value
        exit $value
        """
        % environs
    )
    if pexpect.returncode != 0:
        raise RuntimeError("Tarball copy failed")


def extract_sdk_tarball(adk_root):
    """Extract copied SDK tarball file"""
    sdk_path = os.path.join(adk_root, "External", "raspi-sdk")
    shutil.rmtree(sdk_path, ignore_errors=True)
    os.makedirs(sdk_path)
    f = tarfile.open(".tmp/raspi-sdk.tar.bz2")
    f.extractall(sdk_path)
    shutil.rmtree(".tmp")


def fix_symbolic_links(adk_root):
    """Replace absolute path symbolic links to relative path"""
    root_dir = os.path.join(adk_root, "External", "raspi-sdk")
    for root, dirs, files in os.walk(root_dir):
        for f in files:
            path = os.path.join(root, f)
            if os.path.islink(path):
                link = os.readlink(path)
                if link.startswith("/"):
                    # absolute link. change
                    targetpath = os.path.join(root_dir, link[1:])
                    relativepath = os.path.relpath(targetpath, root)
                    print('Correct "%s": "%s" -> "%s"' % (path, link, relativepath))
                    os.remove(path)
                    os.symlink(relativepath, path)


def delete_tarball_from_raspi(environs):
    """Delete tarball from Raspberry Pi"""
    pexpect = subprocess.Popen("expect", shell=True, stdin=subprocess.PIPE)
    pexpect.communicate(
        """
        set timeout -1
        spawn ssh %(ssh_opts)s  pi@%(hostname)s.local
        expect {
            "password: " {
                send "%(password)s\\n"
                exp_continue
            }
            "pi@%(hostname)s"
        }
        send "sudo rm -rf /temp \\n"
        expect "pi@%(hostname)s"
        send "exit\\n"
        expect eof
        """
        % environs
    )
    if pexpect.returncode != 0:
        raise RuntimeError("Tarball removal failed")


def main():
    parser = argparse.ArgumentParser(
        description="Import Raspberry Pi development header files and libraries into the local compiler"
    )
    parser.add_argument(
        "-n", "--hostname", default="raspberrypi", help="Raspberry Pi host name"
    )
    parser.add_argument("-p", "--password", help="Raspberry Pi password")
    parser.add_argument("-i", "--identity", help="ssh identity file")

    args = parser.parse_args()
    environs = build_environs(args.identity, args.hostname, args.password)
    adk_root = os.path.join(os.path.dirname(sys.argv[0]), "..")

    create_sdk_tarball(environs)
    copy_sdk_tarball(environs)
    extract_sdk_tarball(adk_root)
    fix_symbolic_links(adk_root)
    delete_tarball_from_raspi(environs)


if __name__ == "__main__":
    main()
