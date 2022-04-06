########################################################################################################################
# This is a helper function to download a file from a given URL
#
# Globals:
#   None
# Arguments:
#   $1: file
#   $2: url
#   $3: SHA-256 hash (Optional)
#   $4: Header information as space separate string (Optional)
#   $5: Insecure download (Optional)
# Returns:
#   None
########################################################################################################################
download() {
    if [ "$#" -lt 2 ]; then
        echo "Usage: $0 <file_path> <url> <sha>"
        exit 1
    fi

    # Install uuidgen if not installed already
    # uuidgen is pre-installed on OSX
    if [[ "$(uname)" == "Linux" ]]; then
        if ! uuidgen --version > /dev/null 2>&1; then
            apt-get update
            apt-get install uuid-runtime
        fi
    fi

    DIR=$(dirname "$1")
    mkdir -p "${DIR}"
    local FILE="$1"

    # Check if user specified additional header options
    local HEADER_CURL
    local HEADER_WGET
    if [[ -n "${4:-}" ]]; then
        # Split space separated header arguments into command line args for curl/wget
        IFS=' ' read -ra HEADERS <<< "$4"
        for HEADER in "${HEADERS[@]}"; do
            HEADER_CURL+="-H $HEADER "
            HEADER_WGET+="--header=$HEADER "
        done
    fi

    # Check if user asked for insecure download
    local INSECURE_CURL
    local INSECURE_WGET
    if [[ -n "${5:-}" && $5 == "insecure" ]]; then
        INSECURE_CURL="-k"
        INSECURE_WGET=("--no-check-certificate" "--auth-no-challenge")
    fi

    (
        local lockFile="${FILE}.lock"
        local uuid
        uuid="$(uuidgen)"
        trap '[ "$(cat "'"${lockFile}"'")" == "'"${uuid}"'" ] && rm -f "'"${lockFile}"'"' ERR EXIT
        while ! ( set -C; echo "${uuid}" > "${lockFile}" ) 2>/dev/null; do echo -n "." && sleep 0.5; done

        if [[ -f "${FILE}" && -n "${3:-}" ]]; then
            (echo "$3 *${FILE}" | shasum -a 256 -cw) || rm "${FILE}"
        fi
        if [ ! -f "${FILE}" ]; then
            # URL encode: http://stackoverflow.com/a/7506695
            local url=$2

            # Download & check hash.
            # This is known to abort with `curl: (56) LibreSSL SSL_read: SSL_ERROR_SYSCALL, errno 54`
            # when multiple downloads are started in quick succession. Therefore, retry loop.
            local retryBackoff=0
            local i=0
            while :; do
                if (( ++i > 10 )); then
                    echo "${1} download failed too often."
                    false
                fi
                if (( !retryBackoff )); then
                    retryBackoff=1
                else
                    echo "Retrying ${1} download in ${retryBackoff}s."
                    sleep "${retryBackoff}"
                    retryBackoff=$(( retryBackoff * 2 ))
                    maximumBackoff=1800
                    if (( retryBackoff >= maximumBackoff )); then
                        retryBackoff=${maximumBackoff}
                    fi
                fi

                if command -v curl > /dev/null; then
                    local command=(
                        "curl" ${INSECURE_CURL:+"$INSECURE_CURL"} ${HEADER_CURL:+$HEADER_CURL} "-L" "${url}" "-o" "${FILE}")
                    if command -v caffeinate >/dev/null; then
                        caffeinate -ims "${command[@]}" || continue
                    else
                        "${command[@]}" || continue
                    fi
                elif command -v wget > /dev/null; then
                    # shellcheck disable=SC2086
                    wget ${INSECURE_WGET:+${INSECURE_WGET[*]}} ${HEADER_WGET:+$HEADER_WGET} -O "${FILE}" "${url}" || continue
                else
                    echo -e "\x1B[31m/!\ No download tool installed. Please install curl or wget.\x1B[0m"
                    false
                fi
                break
            done

            if [[ -n "$3" ]]; then
                echo "$3 *${FILE}" | shasum -a 256 -cw
            fi
        fi
    )
}

########################################################################################################################
# This is a helper function to download a file from a given URL over an insecure connection
#
# Globals:
#   None
# Arguments:
#   $1: file
#   $2: url
#   $3: SHA-256 hash (Optional)
#   $4: Header information as space separate string (Optional)
# Returns:
#   None
########################################################################################################################
download_insecure() {
    download "$1" "$2" "$3" "$4" insecure
}
