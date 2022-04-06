#!/usr/bin/env python3

from __future__ import print_function
import argparse
import os
import shutil
import subprocess
import sys

_bluez_package_version = "5.50"
_bluez_package_name = "bluez-" + _bluez_package_version
_patch_dir_name = "raspi_patches"
_patch_version_file_name = "raspi_patch.version"
_patch_diff_file_name = "raspi_bluez.patch"
_patch_version_midfix = "-a"

with open(
    os.path.join(os.path.dirname(__file__), _patch_dir_name, _patch_version_file_name),
    "rt",
    encoding="utf-8",
) as inf:
    _adk_patch_version = (
        _bluez_package_version + _patch_version_midfix + inf.readline().rstrip()
    )

with open(
    os.path.join(os.path.dirname(__file__), _patch_dir_name, _patch_diff_file_name),
    "rt",
    encoding="utf-8",
) as inf:
    _patch = inf.read()


def check_host():
    """Check host machine this script is running on and exit the script if the host machine is not Raspberry Pi"""
    if sys.platform.startswith("linux"):
        release_info_file = "/etc/os-release"
        if os.path.exists(release_info_file):
            with open(release_info_file, "rt", encoding="utf-8") as info_file:
                info = info_file.read().splitlines()

                def get_key_value(s):
                    """Get key and value from release info line"""
                    tokens = s.split("=")
                    key = tokens[0]
                    value = "=".join(tokens[1:])
                    if (value.startswith('"') and value.endswith('"')) or (
                        value.startswith("'") and value.endswith("'")
                    ):
                        value = value[1:-1]
                    return key, value

                info_map = {
                    key: value
                    for (key, value) in [get_key_value(entry) for entry in info]
                }
                if "NAME" in info_map and info_map["NAME"] == "Raspbian GNU/Linux":
                    # Host is Raspberry Pi.
                    return

    print(
        "ERROR: This script must be run on Raspberry Pi only.\n"
        + "       Invoke this script through raspi_bluez_install.py on your host machine.",
        file=sys.stderr,
    )
    sys.exit(1)


def check_version():
    """Check bluez patch version
    This function returns True if version matches.
    """
    status = (
        subprocess.check_output("sudo systemctl status bluetooth", shell=True)
        .decode()
        .strip()
    )
    if (
        status.find(
            "/usr/local/libexec/bluetooth/bluetoothd -d --experimental --noplugin=battery"
        )
        < 0
    ):
        return False
    version = (
        subprocess.check_output(
            "/usr/local/libexec/bluetooth/bluetoothd --version", shell=True
        )
        .decode()
        .strip()
    )

    return version == _adk_patch_version


def update_version_tag():
    """Update version tag of the BlueZ package"""
    subprocess.check_call(
        "sed -i \"s/^ VERSION='%s'$/ VERSION='%s'/\" configure"
        % (_bluez_package_version, _adk_patch_version),
        shell=True,
    )


def done():
    """Print a notice message"""
    print("Done.")
    print(
        "Note that every time Raspberry Pi reboots, the following command must be executed:"
    )
    print("sudo systemctl restart bluetooth")


def install(force):
    if os.path.exists(_bluez_package_name):
        shutil.rmtree(_bluez_package_name)
    print("Installing dependencies...")
    subprocess.check_call("sudo apt update --allow-releaseinfo-change", shell=True)
    subprocess.check_call(
        "sudo apt install libdbus-1-dev libudev-dev libical-dev libreadline-dev -y",
        shell=True,
    )
    if force or not os.path.exists(_bluez_package_name + ".tar.xz"):
        print("Downloading bluez stack...")
        subprocess.check_call(
            "wget http://www.kernel.org/pub/linux/bluetooth/"
            + _bluez_package_name
            + ".tar.xz",
            shell=True,
        )
    print("Extracting bluez stack...")
    subprocess.check_call("tar xvf " + _bluez_package_name + ".tar.xz", shell=True)
    print("Building bluez stack...")
    os.chdir(_bluez_package_name)
    p = subprocess.Popen(
        "patch -p1", stdin=subprocess.PIPE, shell=True, encoding="utf-8"
    )
    p.communicate(_patch)
    if p.returncode != 0:
        sys.exit(1)
    update_version_tag()
    subprocess.check_call(
        "./configure --enable-library CFLAGS=-pthread LIBS=-pthread", shell=True
    )
    subprocess.check_call("make", shell=True)
    subprocess.check_call("sudo make install", shell=True)
    print("Restarting bluetooth service...")
    subprocess.check_call("sudo systemctl daemon-reload", shell=True)
    subprocess.check_call("sudo systemctl restart bluetooth", shell=True)
    os.chdir("..")
    shutil.copyfile(
        os.path.join(
            os.path.dirname(__file__), _patch_dir_name, _patch_version_file_name
        ),
        os.path.join(_bluez_package_name, _patch_version_file_name),
    )


def main():
    check_host()
    parser = argparse.ArgumentParser(description="Reinstall bluez stack with ADK patch")
    parser.add_argument(
        "--force",
        action="store_true",
        help="Force downloading and installing bluez stack regardless of the current state.",
    )
    args = parser.parse_args()

    if not args.force:
        if check_version():
            print("Patched bluez source files were found. Install is aborted.")
            print("Restarting bluetooth service...")
            subprocess.check_call("sudo systemctl restart bluetooth", shell=True)
            done()
            return
    install(args.force)
    done()


if __name__ == "__main__":
    main()
