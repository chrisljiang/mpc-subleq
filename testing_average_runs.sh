#!/bin/bash

# expected number of runs performed
CHK=true
EXP_NO='50'
EXP_SIM='25'

# change working directory to location of script
cd "$(dirname "${0}")"

# subleq no-internet
CUR_FILE='subleq_no-internet.csv'
echo 'data_size,iterations,addresses_per_instruction,time/μs' > "${CUR_FILE}"
while IFS= read -r FILE ; do
    SUM='0'
    CNT="$(grep 'MPC run' "${FILE}" | wc -l)"
    if [[ "${CHK}" = true && "${CNT}" -ne "${EXP_NO}" ]] ; then
        echo "Error on file: ${FILE}, got ${CNT} but expected ${EXP_NO}"
        continue
    fi
    while IFS= read -r LINE ; do
        SUM="$((SUM + ${LINE}))"
    done < <(grep -Po '(?<=MPC run \(nano\): )[0-9]+' "${FILE}")
    AVG="$(echo "scale=5 ; ${SUM} / ${CNT} / 1000" | bc)"  # convert nanoseconds to microseconds
    echo "${FILE}-${AVG}" | awk --field-separator '-' '{printf("%d,%d,%d,%.5f\n", $(NF-4), $(NF-3), $(NF-2), $NF)}' >> "${CUR_FILE}"
done < <(find ./testing_subleq-mpc/results -name 'no-internet*p0.log' | awk --field-separator '-' '{printf("%d %d %d %s\n", $(NF-3), $(NF-2), $(NF-1), $0)}' | sort -n -k1 -k2 -k3 | awk '{print $NF}')

# gc-lite no-internet
CUR_FILE='gc-lite_no-internet.csv'
echo 'data_size,prog_size,total_size,iterations,time/μs' > "${CUR_FILE}"
while IFS= read -r FILE ; do
    SUM='0'
    CNT="$(grep 'Execution Time' "${FILE}" | wc -l)"
    TOT="$(($(echo "${FILE}" | awk --field-separator '-' '{print $(NF-3)}') + $(echo "${FILE}" | awk --field-separator '-' '{print $(NF-2)}')*4))"
    if [[ "${CHK}" = true && "${CNT}" -ne "${EXP_NO}" ]] ; then
        echo "Error on file: ${FILE}, got ${CNT} but expected ${EXP_NO}"
        continue
    fi
    while IFS= read -r LINE ; do
        SUM="$((SUM + ${LINE}))"
    done < <(grep -Po '(?<=Execution Time = )[0-9]+(?= microseconds)' "${FILE}")
    AVG="$(echo "scale=2 ; ${SUM} / ${CNT}" | bc)"
    echo "${FILE}-${AVG}-${TOT}" | awk --field-separator '-' '{printf("%d,%d,%d,%d,%.2f\n", $(NF-5), $(NF-4), $NF, $(NF-3), $(NF-1))}' >> "${CUR_FILE}"
done < <(find ./testing_gc-lite/results -name 'no-internet*alice.log' | awk --field-separator '-' '{printf("%d %d %d %s\n", $(NF-3), $(NF-2), $(NF-1), $0)}' | sort -n -k1 -k2 -k3 | awk '{print $NF}')

# subleq simulated-internet
CUR_FILE='subleq_simulated-internet.csv'
echo 'bandwidth/Mbps,latency/ms,data_size,iterations,addresses_per_instruction,time/μs' > "${CUR_FILE}"
while IFS= read -r FILE ; do
    SUM='0'
    CNT="$(grep 'MPC run' "${FILE}" | wc -l)"
    if [[ "${CHK}" = true && "${CNT}" -ne "${EXP_SIM}" ]] ; then
        echo "Error on file: ${FILE}, got ${CNT} but expected ${EXP_SIM}"
        continue
    fi
    while IFS= read -r LINE ; do
        SUM="$((SUM + ${LINE}))"
    done < <(grep -Po '(?<=MPC run \(nano\): )[0-9]+' "${FILE}")
    AVG="$(echo "scale=5 ; ${SUM} / ${CNT} / 1000" | bc)"  # convert nanoseconds to microseconds
    echo "${FILE}-${AVG}" | awk --field-separator '[-/]' '{printf("%s,%.1f,%d,%d,%d,%.5f\n", $(NF-6), $(NF-5), $(NF-4), $(NF-3), $(NF-2), $NF)}' | sed -E 's/^n/-/g' >> "${CUR_FILE}"
done < <(find ./testing_subleq-mpc/results -name '*p0.log' | grep -v 'no-internet' | awk --field-separator '[-/]' '{printf("%s %.1f %d %d %d %s\n", $(NF-5), $(NF-4), $(NF-3), $(NF-2), $(NF-1), $0)}' | sed -E 's/^n/-/g' | sort -n -k1 -k2 -k3 -k4 -k5 | awk '{print $NF}')

# gc-lite simulated-internet
CUR_FILE='gc-lite_simulated-internet.csv'
echo 'bandwidth/Mbps,latency/ms,data_size,prog_size,total_size,iterations,time/μs' > "${CUR_FILE}"
while IFS= read -r FILE ; do
    SUM='0'
    CNT="$(grep 'Execution Time' "${FILE}" | wc -l)"
    TOT="$(($(echo "${FILE}" | awk --field-separator '-' '{print $(NF-3)}') + $(echo "${FILE}" | awk --field-separator '-' '{print $(NF-2)}')*4))"
    if [[ "${CHK}" = true && "${CNT}" -ne "${EXP_SIM}" ]] ; then
        echo "Error on file: ${FILE}, got ${CNT} but expected ${EXP_SIM}"
        continue
    fi
    while IFS= read -r LINE ; do
        SUM="$((SUM + ${LINE}))"
    done < <(grep -Po '(?<=Execution Time = )[0-9]+(?= microseconds)' "${FILE}")
    AVG="$(echo "scale=2 ; ${SUM} / ${CNT}" | bc)"
    echo "${FILE}-${AVG}-${TOT}" | awk --field-separator '[-/]' '{printf("%s,%.1f,%d,%d,%d,%d,%.2f\n", $(NF-7), $(NF-6), $(NF-5), $(NF-4), $NF, $(NF-3), $(NF-1))}' | sed -E 's/^n/-/g' >> "${CUR_FILE}"
done < <(find ./testing_gc-lite/results -name '*alice.log' | grep -v 'no-internet' | awk --field-separator '[-/]' '{printf("%s %.1f %d %d %d %s\n", $(NF-5), $(NF-4), $(NF-3), $(NF-2), $(NF-1), $0)}' | sed -E 's/^n/-/g' | sort -n -k1 -k2 -k3 -k4 -k5 | awk '{print $NF}')
