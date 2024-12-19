#!/bin/bash

# subleq-mpc database size
MIN_SIZE="${1}"
INC_SIZE="${2}"
MAX_SIZE="${3}"

# gc-lite database size
MIN_DATA="${4}"
INC_DATA="${5}"
MAX_DATA="${6}"

# gc-lite program size
MIN_PROG="${7}"
INC_PROG="${8}"
MAX_PROG="${9}"

# number of instructions to run
MIN_INST="${10}"
INC_INST="${11}"
MAX_INST="${12}"

# number of repetitions of each test
RUNS="${13}"

# change working directory to location of script
cd "$(dirname "${0}")"

# test subleq-mpc
cd ./testing_subleq-mpc
if [ ! -d 'results' ]; then
    mkdir results
fi
# ./build.sh
for SIZE in $(seq "${MIN_SIZE}" "${INC_SIZE}" "${MAX_SIZE}") ; do
    for INST in $(seq "${MIN_INST}" "${INC_INST}" "${MAX_INST}") ; do
        for ADDR in "3" "4" ; do
            PRE="no-internet-${SIZE}-${INST}-${ADDR}-"
            if [[ ! -e "./results/${PRE}p2.log" ]] ; then
                touch "./results/${PRE}p2.log"
                ./run.sh -t \
                         -m "preprocessing" \
                         -n "${SIZE}" \
                         -i "${INST}" \
                         -a "${ADDR}" \
                         -s "${RUNS}" \
                         -p "${PRE}"
            fi
        done
    done
done
# ./clean.sh

# test gc-lite
cd ../testing_gc-lite
if [ ! -d 'results' ]; then
    mkdir results
fi
# ./build.sh
for DATA in $(seq "${MIN_DATA}" "${INC_DATA}" "${MAX_DATA}") ; do
    for PROG in $(seq "${MIN_PROG}" "${INC_PROG}" "${MAX_PROG}") ; do
        for INST in $(seq "${MIN_INST}" "${INC_INST}" "${MAX_INST}") ; do
            PRE="no-internet-${DATA}-${PROG}-${INST}-"
            if [[ ! -e "./results/${PRE}alice.log" ]] ; then
                touch "./results/${PRE}alice.log"
                ./run.sh -t \
                         -n "${DATA}" \
                         -m "${PROG}" \
                         -i "${INST}" \
                         -s "${RUNS}" \
                         -p "${PRE}"
            fi
        done
    done
done
# ./clean.sh
