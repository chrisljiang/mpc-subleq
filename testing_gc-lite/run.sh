#!/bin/bash

# print usage information and exit
usage()
{
    echo "Usage: ${0} [OPTIONS]"
    echo "Options:"
    echo "    -n <DATA>  - size of database"
    echo "    -m <PROG>  - size of program"
    echo "    -i <INST>  - number of instruction to execute"
    echo "    -r <RATE>  - new rate (negative value for unlimited bandwidth)"
    echo "    -d <DELAY> - new delay"
    echo "    -p <PRE>   - container and log file prefix"
    echo "    -s <RUNS>  - runs to perform"
    echo "    -t         - disable tc usage"
    exit "${1}"
}

# set defaults to optional command-line arguments
DATA="10"
PROG="10"
INST="100"
RATE="100mbit"
DELAY="10ms"
PRE=""
RUNS="1"
TC="ON"

# get optional command-line arguments
while getopts ":n:m:i:r:d:p:s:th" ARG ; do
    case "${ARG}" in
        n) DATA="${OPTARG}" ;;
        m) PROG="${OPTARG}" ;;
        i) INST="${OPTARG}" ;;
        r) RATE="${OPTARG}" ;;
        d) DELAY="${OPTARG}" ;;
        p) PRE="${OPTARG}" ;;
        s) RUNS="${OPTARG}" ;;
        t) TC="" ;;
        h) usage "0" ;;
        :) echo "'${OPTARG}' missing required argument" ; usage "1" ;;
        \?) echo "'${OPTARG}' option not found" ; usage "1" ;;
    esac
done

# change working directory to location of script
cd "$(dirname "${0}")"

# verify data size, code size, and number of instructions to run
DATA_MIN="2"
PROG_MIN="2"
INST_MIN="1"
if [[ "${DATA}" -lt "${DATA_MIN}" ]] ; then
    echo "'${DATA}' is invalid, must be an integer greater than or equal to ${DATA_MIN}"
    usage "1"
fi
if [[ "${PROG}" -lt "${PROG_MIN}" ]] ; then
    echo "'${PROG}' is invalid, must be an integer greater than or equal to ${PROG_MIN}"
    usage "1"
fi
if [[ "${INST}" -lt "${INST_MIN}" ]] ; then
    echo "'${INST}' is invalid, must be an integer greater than or equal to ${INST_MIN}"
    usage "1"
fi

# create directory for results
if [ ! -d 'results' ]; then
    mkdir results
fi

# run alice and set traffic control
docker container run --name "${PRE}alice" \
                     --detach \
                     --rm \
                     --mount 'type=bind,source=./results,destination=/results' \
                     testing:gc-lite \
                     /bin/bash -c "truncate --size=0 /results/${PRE}alice.log ; for i in \$(seq ${RUNS}) ; do python3 /container_files/recv.py 0 >> /results/${PRE}alice.log 2>&1 ; echo \"Starting alice: \${i}\" >> /results/${PRE}alice.log 2>&1 ; ./alice >> /results/${PRE}alice.log 2>&1 ; sleep 0.1s ; done"
if [[ -n "${TC}" ]] ; then
    if [[ "${RATE}" =~ ^- ]] ; then
        ../traffic-control.sh -d "${DELAY}" "${PRE}alice"
    else
        ../traffic-control.sh -r "${RATE}" -d "${DELAY}" "${PRE}alice"
    fi
fi

# run helper and bob attached to alice's local network
docker container run --name "${PRE}helper" \
                     --detach \
                     --rm \
                     --mount 'type=bind,source=./results,destination=/results' \
                     --network "container:${PRE}alice" \
                     testing:gc-lite \
                     /bin/bash -c "truncate --size=0 /results/${PRE}helper.log ; for i in \$(seq ${RUNS}) ; do /container_files/setup.sh ${DATA} ${PROG} ${INST} >> /results/${PRE}helper.log 2>&1 ; python3 /container_files/send.py >> /results/${PRE}helper.log 2>&1 ; sleep 0.1s ; done"
docker container run --name "${PRE}bob" \
                     -it \
                     --rm \
                     --mount 'type=bind,source=./results,destination=/results' \
                     --network "container:${PRE}alice" \
                     testing:gc-lite \
                     /bin/bash -c "truncate --size=0 /results/${PRE}bob.log ; for i in \$(seq ${RUNS}) ; do python3 /container_files/recv.py 1 2>&1 | tee -a /results/${PRE}bob.log ; sleep 0.1s ; echo \"Starting bob: \${i}\" 2>&1 | tee -a /results/${PRE}bob.log ; time ./bob 2>&1 | tee -a /results/${PRE}bob.log ; sleep 0.1s ; done"
