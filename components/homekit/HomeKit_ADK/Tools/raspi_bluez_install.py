#!/usr/bin/env python3

from __future__ import print_function

import argparse
import os
import subprocess


def install(hostname, hostaddress, username, password):
    """Install bluez stack"""

    # Common arguments
    args = {
        "tools_path": os.path.dirname(__file__),
        "hostname": hostname,
        "hostaddress": hostaddress,
        "username": username,
        "password": password,
    }

    # Copy bluez patch script file to Raspberry Pi
    subprocess.check_call(
        (
            """expect <<EOF
         set timeout -1
         spawn scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -r \
             %(tools_path)s/patch_raspi_bluez.py %(tools_path)s/raspi_patches \
             %(username)s@%(hostaddress)s:/home/%(username)s/
         expect {
           "password:" { send "%(password)s\\n"; exp_continue }
           eof
         }
         lassign [wait] pid spawnID osError value
         exit \\$value
EOF"""
        )
        % args,
        shell=True,
    )

    subprocess.check_call(
        (
            """expect <<EOF
         set timeout -1
         spawn ssh -o ConnectTimeout=30 -o ServerAliveInterval=10000 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
             %(username)s@%(hostaddress)s "cd /home/%(username)s && ./patch_raspi_bluez.py && rm -fr raspi_patches patch_raspi_bluez.py"
         expect {
           "password:" { send "%(password)s\\n"; exp_continue }
           eof
         }
         lassign [wait] pid spawnID osError value
         exit \\$value
EOF"""
        )
        % args,
        shell=True,
    )


def generate_header(header_file):
    input_file_path = os.path.join(
        os.path.dirname(__file__), "raspi_patches", "raspi_patch.version"
    )
    with open(input_file_path, "rt") as input:
        version_string = "5.50-a" + "".join([x.strip() for x in input.readlines()])
        with open(header_file, "wt") as output:
            output.write("#ifndef BLUEZ_VERSION_STRING\n")
            output.write('#define BLUEZ_VERSION_STRING "%s"\n' % version_string)
            output.write("#endif\n")
            output.flush()


def main():
    parser = argparse.ArgumentParser(
        description="Install bluez stack interoperable with iOS Home"
    )
    parser.add_argument(
        "-n",
        "--hostname",
        default="raspberrypi",
        help='Raspberry Pi hostname. Default is "raspberrypi".',
    )
    parser.add_argument(
        "-N",
        "--hostaddress",
        default=None,
        help="Raspberry Pi numeric host address.",
    )
    parser.add_argument(
        "-u", "--user", default="pi", help='Raspberry Pi username. Default is "pi".'
    )
    parser.add_argument(
        "-p",
        "--password",
        default="raspberry",
        help='Raspberry Pi user password. Default is "raspberry".',
    )
    parser.add_argument(
        "--generate-header",
        help="Generate a designated version header file from the patch version",
    )

    args = parser.parse_args()

    if args.generate_header is not None:
        generate_header(args.generate_header)
        return

    install(
        args.hostname,
        args.hostname + ".local" if args.hostaddress is None else args.hostaddress,
        args.user,
        args.password,
    )


if __name__ == "__main__":
    main()
