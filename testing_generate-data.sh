#!/bin/bash

# bandwidth
MIN_BAND="${1}"
INC_BAND="${2}"
MAX_BAND="${3}"

# latency
MIN_LATE="${4}"
INC_LATE="${5}"
MAX_LATE="${6}"

# subleq-mpc database size
MIN_SIZE="${7}"
INC_SIZE="${8}"
MAX_SIZE="${9}"

# gc-lite database size
MIN_DATA="${10}"
INC_DATA="${11}"
MAX_DATA="${12}"

# gc-lite program size
MIN_PROG="${13}"
INC_PROG="${14}"
MAX_PROG="${15}"

# number of instructions to run
MIN_INST="${16}"
INC_INST="${17}"
MAX_INST="${18}"

# number of repetitions of each test
RUNS="${19}"

# change working directory to location of script
cd "$(dirname "${0}")"

# test subleq-mpc
cd ./testing_subleq-mpc
if [ ! -d 'results' ]; then
    mkdir results
fi
# ./build.sh
for BAND in $(seq "${MIN_BAND}" "${INC_BAND}" "${MAX_BAND}") ; do
    for LATE in $(seq "${MIN_LATE}" "${INC_LATE}" "${MAX_LATE}") ; do
        for SIZE in $(seq "${MIN_SIZE}" "${INC_SIZE}" "${MAX_SIZE}") ; do
            for INST in $(seq "${MIN_INST}" "${INC_INST}" "${MAX_INST}") ; do
                for ADDR in "3" "4" ; do
                    PRE="${BAND}-${LATE}-${SIZE}-${INST}-${ADDR}-"
                    PRE="$(echo "${PRE}" | sed -E 's/^-/n/g')"
                    if [[ ! -e "./results/${PRE}p2.log" ]] ; then
                        touch "./results/${PRE}p2.log"
                        ./run.sh -m "preprocessing" \
                                 -n "${SIZE}" \
                                 -i "${INST}" \
                                 -a "${ADDR}" \
                                 -r "${BAND}mbit" \
                                 -d "${LATE}ms" \
                                 -s "${RUNS}" \
                                 -p "${PRE}"
                    fi
                done
            done
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
for BAND in $(seq "${MIN_BAND}" "${INC_BAND}" "${MAX_BAND}") ; do
    for LATE in $(seq "${MIN_LATE}" "${INC_LATE}" "${MAX_LATE}") ; do
        for DATA in $(seq "${MIN_DATA}" "${INC_DATA}" "${MAX_DATA}") ; do
            for PROG in $(seq "${MIN_PROG}" "${INC_PROG}" "${MAX_PROG}") ; do
                for INST in $(seq "${MIN_INST}" "${INC_INST}" "${MAX_INST}") ; do
                    PRE="${BAND}-${LATE}-${DATA}-${PROG}-${INST}-"
                    PRE="$(echo "${PRE}" | sed -E 's/^-/n/g')"
                    if [[ ! -e "./results/${PRE}alice.log" ]] ; then
                        touch "./results/${PRE}alice.log"
                        ./run.sh -n "${DATA}" \
                                 -m "${PROG}" \
                                 -i "${INST}" \
                                 -r "${BAND}mbit" \
                                 -d "${LATE}ms" \
                                 -s "${RUNS}" \
                                 -p "${PRE}"
                    fi
                done
            done
        done
    done
done
# ./clean.sh
