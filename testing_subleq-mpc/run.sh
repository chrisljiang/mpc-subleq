#!/bin/bash

# print usage information and exit
usage()
{
    echo "Usage: ${0} [OPTIONS]"
    echo "Options:"
    echo "    -m <MODE>  - must be 'preprocessing' or 'online_only'"
    echo "    -n <SIZE>  - size of database"
    echo "    -i <INST>  - number of instruction to execute"
    echo "    -a <ADDR>  - addresses per instruction"
    echo "    -r <RATE>  - new rate (negative value for unlimited bandwidth)"
    echo "    -d <DELAY> - new delay"
    echo "    -p <PRE>   - container and log file prefix"
    echo "    -s <RUNS>  - runs to perform"
    echo "    -c <CHECK> - frequency of OOB checks"
    echo "    -f <PRINT> - frequency of round info prints"
    echo "    -b <BUF>   - number of rounds to buffer for 'online_only' mode"
    echo "    -t         - disable tc usage"
    exit "${1}"
}

# set defaults to optional command-line arguments
MODE="preprocessing"
SIZE="10"
INST="100"
ADDR="3"
RATE="100mbit"
DELAY="10ms"
PRE=""
RUNS="1"
CHECK="0"
PRINT="100"
BUF="50"
TC="ON"

# get optional command-line arguments
while getopts ":m:n:i:a:r:d:p:s:c:f:b:th" ARG ; do
    case "${ARG}" in
        m) MODE="${OPTARG}" ;;
        n) SIZE="${OPTARG}" ;;
        i) INST="${OPTARG}" ;;
        a) ADDR="${OPTARG}" ;;
        r) RATE="${OPTARG}" ;;
        d) DELAY="${OPTARG}" ;;
        p) PRE="${OPTARG}" ;;
        s) RUNS="${OPTARG}" ;;
        c) CHECK="${OPTARG}" ;;
        f) PRINT="${OPTARG}" ;;
        b) BUF="${OPTARG}" ;;
        t) TC="" ;;
        h) usage "0" ;;
        :) echo "'${OPTARG}' missing required argument" ; usage "1" ;;
        \?) echo "'${OPTARG}' option not found" ; usage "1" ;;
    esac
done

# change working directory to location of script
cd "$(dirname "${0}")"

# verify mode to run in ('preprocessing' or 'online only')
if [[ "${MODE}" != "preprocessing" && "${MODE}" != "online_only" ]] ; then
    echo "'${MODE}' is not a valid mode"
    usage "1"
fi

# verify addresses per instruction
if [[ "${ADDR}" -ne 3 && "${ADDR}" -ne 4 ]] ; then
    echo "'${ADDR}' is not a valid number of address per instruction, must be 3 or 4"
    usage "1"
fi

# verify data size and number of instructions to run
if [[ "${ADDR}" -eq 3 ]] ; then
    SIZE_MIN="7"
    INST_MIN="2"
else
    SIZE_MIN="6"
    INST_MIN="1"
fi

if [[ "${SIZE}" -lt "${SIZE_MIN}" ]] ; then
    echo "'${SIZE}' is invalid, must be an integer greater than or equal to ${SIZE_MIN} when using ${ADDR}-address instructions"
    usage "1"
fi
if [[ "${INST}" -lt "${INST_MIN}" ]] ; then
    echo "'${INST}' is invalid, must be an integer greater than or equal to ${INST_MIN} when using ${ADDR}-address instructions"
    usage "1"
fi

# verify number of runs to perform
if [[ "${RUNS}" -le 0 ]] ; then
    echo "'${RUNS}' is invalid, must be a positive integer, setting to 1"
    RUNS="1"
fi

# verify print rounds
if [[ "${PRINT}" -eq 0 ]] ; then
    echo "'${PRINT}' is invalid, must be a non-zero integer, setting to 100"
    PRINT="100"
fi

# verify buf rounds
if [[ "${BUF}" -eq 0 ]] ; then
    echo "'${BUF}' is invalid, must be a non-zero integer, setting to 50"
    BUF="50"
fi

# create directory for results
if [ ! -d 'results' ]; then
    mkdir results
fi

# run p2 and set traffic control
docker container run --name "${PRE}p2" \
                     --detach \
                     --rm \
                     --mount 'type=bind,source=./results,destination=/results' \
                     testing:subleq-mpc \
                     /bin/bash -c "/subleq-mpc/testing_subleq-mpc/setup.sh p2 ${MODE} ${SIZE} ${INST} ${ADDR} ${CHECK} ${PRINT} ${BUF} > /results/${PRE}p2.log 2>&1 ; for i in \$(seq ${RUNS}) ; do /subleq-mpc/parties_${MODE}/p2 >> /results/${PRE}p2.log 2>&1 ; sleep 0.1s ; done ; tc qdisc show dev lo >> /results/${PRE}p2.log 2>&1"
if [[ -n "${TC}" ]] ; then
    if [[ "${RATE}" =~ ^- ]] ; then
        ../traffic-control.sh -d "${DELAY}" "${PRE}p2"
    else
        ../traffic-control.sh -r "${RATE}" -d "${DELAY}" "${PRE}p2"
    fi
fi

# run p0 and p1 attached to p2's local network
docker container run --name "${PRE}p1" \
                     --detach \
                     --rm \
                     --mount 'type=bind,source=./results,destination=/results' \
                     --network "container:${PRE}p2" \
                     testing:subleq-mpc \
                     /bin/bash -c "/subleq-mpc/testing_subleq-mpc/setup.sh p1 ${MODE} ${SIZE} ${INST} ${ADDR} ${CHECK} ${PRINT} ${BUF} > /results/${PRE}p1.log 2>&1 ; for i in \$(seq ${RUNS}) ; do /subleq-mpc/parties_${MODE}/p1 >> /results/${PRE}p1.log 2>&1 ; sleep 0.1s ; done"
docker container run --name "${PRE}p0" \
                     -it \
                     --rm \
                     --mount 'type=bind,source=./results,destination=/results' \
                     --network "container:${PRE}p2" \
                     testing:subleq-mpc \
                     /bin/bash -c "/subleq-mpc/testing_subleq-mpc/setup.sh p0 ${MODE} ${SIZE} ${INST} ${ADDR} ${CHECK} ${PRINT} ${BUF} 2>&1 | tee /results/${PRE}p0.log ; for i in \$(seq ${RUNS}) ; do /subleq-mpc/parties_${MODE}/p0 2>&1 | tee -a /results/${PRE}p0.log ; sleep 0.1s ; done"
