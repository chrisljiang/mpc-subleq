#!/bin/bash

# get command line arguments
DATA="${1}"
PROG="${2}"
INST="${3}"

# update SUBLEQ code
DATA_FILE="/container_files/D.txt"
PROG_FILE="/container_files/P.txt"
DATA_LINES="$(wc -l ${DATA_FILE} | awk '{ print $1 }')"
PROG_LINES="$(wc -l ${PROG_FILE} | awk '{ print $1 }')"
sed -i -E -e "2s/^[0-9]+/${INST}/" "${DATA_FILE}"
if [[ "${DATA}" -gt "${DATA_LINES}" ]] ; then
    for i in $(seq "${DATA_LINES}" "$((DATA-1))") ; do
        echo "0 ${i}" >> "${DATA_FILE}"
    done
    DATA_LINES="${DATA}"
fi
if [[ "${PROG}" -gt "${PROG_LINES}" ]] ; then
    for i in $(seq "${PROG_LINES}" "$((PROG-1))") ; do
        echo "-1 -1 -1 -1 ${i}" >> "${PROG_FILE}"
    done
    PROG_LINES="${PROG}"
fi

# print updated SUBLEQ code
echo "----- DATA FILE -----"
cat "${DATA_FILE}"
echo "----- PROG FILE -----"
cat "${PROG_FILE}"

# run preprocessor and helper
FILES="${PROG_FILE}\n${DATA_FILE}"
SIZES="${PROG_LINES}\n${DATA_LINES}\n$((INST+1))"
/container_files/run_preprocessor_and_helper.sh "${FILES}" "${SIZES}"
