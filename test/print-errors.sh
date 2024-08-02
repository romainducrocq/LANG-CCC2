#!/bin/bash

PACKAGE_NAME="$(cat ../bin/package_name.txt)"

TEST_DIR="${PWD}/tests"

function print_errors () {
    for FILE in $(find ${TEST_DIR}/${1}_* -name "*.c" -type f | grep invalid)
    do
        cat <(${PACKAGE_NAME} -S ${FILE} 2>&1) | grep -P "${PRINT_PATTERN}"
        if [ -f ${FILE%.*}.s ]; then
             rm ${FILE%.*}.s
        fi
    done
}

ARG=${1}

PRINT_PATTERN=""
if [ "${1}" = "--what" ]; then
    PRINT_PATTERN="(no\. [0-9]+)"
    ARG=${2}
elif [ "${1}" = "--line" ]; then
    PRINT_PATTERN="^at line [0-9]+:"
    ARG=${2}
fi

if [ ! -z "${ARG}" ]; then
    print_errors ${ARG}
else
    for i in $(seq 1 18); do
        print_errors ${i}
    done
fi

exit 0
