#!/bin/bash

if [[ -z "${1}" ]] ; then
    echo "No preprocessor args specified, exiting"
    exit 1
else
    PREPROCESSOR_ARG="${1}"
fi

if [[ -z "${2}" ]] ; then
    echo "No helper args specified, exiting"
    exit 1
else
    HELPER_ARG="${2}"
fi

printf "${PREPROCESSOR_ARG}" | /GC-Lite/GC-Lite/build/preprocessor
printf "${HELPER_ARG}" | /GC-Lite/GC-Lite/build/helper
