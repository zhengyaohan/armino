#!/bin/bash -e
set -eu -o pipefail
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ADK_ROOT="$DIR/../"

usage()
{
    echo "This script is used to copy and build the ADK on another device."
    echo ""
    echo "Usage: $0 -n HOSTNAME -u USERNAME [-i SSH_IDENTITY | -p SSH_PASSWORD] -c MAKEOPTION"
    echo ""
    echo "Example:"
    echo "   $0 -n raspi -p raspberry -u pi -c \"TARGET=Raspi DOCKER=0 PROFILE=Lightbulb apps\""
    echo ""
    echo "OPTIONS:"
    echo "-n  - [required] device host name"
    echo "-u  - [required] device user name"
    echo "-c  - [required] make options"
    echo "-p  - [optional] device password"
    echo "-i  - [optional] ssh identity file"
    echo "-d  - [optional] remote ADK root directory (default: \"~/adk_build\")"
    echo "-e  - [optional] wipes remote folder before doing anything. Can be used by itself or in conjunction with -c command"
    echo ""
    exit 1
}

USERNAME=
HOSTNAME=
PASSWORD=
IDENTITY_FILE=
MAKE_OPTIONS=
ERASE_REMOTE=0
# shellcheck disable=SC2088
REMOTE_DIR="~/adk_build"
while getopts "hu:n:i:p:c:ed:" opt; do
    case ${opt} in
        u ) USERNAME=$OPTARG;;
        n ) HOSTNAME=$OPTARG;;
        p ) PASSWORD=$OPTARG;;
        i ) IDENTITY_FILE=$OPTARG;;
        c ) MAKE_OPTIONS=$OPTARG;;
        d ) REMOTE_DIR=$OPTARG;;
        e ) ERASE_REMOTE=1;;
        h ) usage;;
        \? ) usage;;
    esac
done

if [[ -z "$HOSTNAME" ]]; then
    usage
fi

if [[ -z "$MAKE_OPTIONS" && "$ERASE_REMOTE" -eq 0 ]]; then
    usage
fi

SSH_OPTS=(-o 'StrictHostKeyChecking=no' -o 'UserKnownHostsFile=/dev/null' -o 'ConnectTimeout=30' -o 'ServerAliveInterval=10000')
if [ -n "$IDENTITY_FILE" ]; then
    SSH_OPTS+=(-i "$IDENTITY_FILE")
fi

if [[ $ERASE_REMOTE -eq 1 ]]; then
    echo "Erasing remote build directory"
    echo ""

    expect <<EOF
    set timeout -1
    spawn ssh ${SSH_OPTS[@]} ${USERNAME}@${HOSTNAME}.local
    expect {
        "password: " {
            send "${PASSWORD}\n"
            exp_continue
        }
        "${USERNAME}@${HOSTNAME}"
    }
    send "rm -rf ${REMOTE_DIR}\n"
    expect "${USERNAME}@${HOSTNAME}"
    send "exit\n"
    expect eof
EOF
    echo "Done."
    echo ""
    if [[ -z "$MAKE_OPTIONS" ]]; then
        exit
    fi
fi

EXCLUDE_LIST=(--exclude "Output/*" --exclude "private/*"  --exclude "rio/*" --exclude "*.img" --exclude "\.*")

expect <<EOF
    set timeout -1
    spawn rsync -e "ssh ${SSH_OPTS[@]}" -crlpgoD ${EXCLUDE_LIST[@]} --verbose $ADK_ROOT ${USERNAME}@${HOSTNAME}.local:${REMOTE_DIR}/
    expect {
        "password: " {
            send "${PASSWORD}\n"
            exp_continue
        }
        "building file list" {
            exp_continue
        }
        "total size"
    }
EOF

expect <<EOF
    set timeout -1
    spawn ssh ${SSH_OPTS[@]} ${USERNAME}@${HOSTNAME}.local
    expect {
        "password: " {
            send "${PASSWORD}\n"
            exp_continue
        }
        "${USERNAME}@${HOSTNAME}"
    }
    send "make -C ${REMOTE_DIR}/ ${MAKE_OPTIONS} \n"
    expect "${USERNAME}@${HOSTNAME}"
    send "exit \$?\n"
    expect eof
    lassign [wait] pid spawnID osError value
    exit \$value
EOF

expect <<EOF
    set timeout -1
    spawn rsync -e "ssh ${SSH_OPTS[@]}" -crlpgoD --verbose ${USERNAME}@${HOSTNAME}.local:${REMOTE_DIR}/Output ${ADK_ROOT}/
    expect {
        "password: " {
            send "${PASSWORD}\n"
            exp_continue
        }
        "building file list" {
            exp_continue
        }
        "total size"
    }
EOF
