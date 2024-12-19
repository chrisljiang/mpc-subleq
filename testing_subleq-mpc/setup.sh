#!/bin/bash

# get command line arguments
PARTY="${1}"
MODE="${2}"
SIZE="${3}"
INST="${4}"
ADDR="${5}"
CHECK="${6}"
PRINT="${7}"
BUF="${8}"

# update SUBLEQ code
FILE="/subleq-mpc/sample_code/tight_loop_nonuniform_${ADDR}.sq"
if [[ "${ADDR}" -eq 3 ]] ; then
    sed -i -E -e "1s/^[0-9]+/${SIZE}/" \
              -e "\$s/^-?[0-9]+/-$((INST-2))/" "${FILE}"
else
    sed -i -E -e "1s/^[0-9]+/${SIZE}/" \
              -e "\$s/^-?[0-9]+/${INST}/" "${FILE}"
fi

# update constants.hpp based on program to run and build
if [[ "${MODE}" == "preprocessing" ]] ; then
    perl -0777 -pi -e "s/num_rounds = [0-9]+/num_rounds = $((INST+1))/;" \
                   -e "s/data_size = [0-9]+/data_size = ${SIZE}/;" \
                   -e "s/data_per_code = (3|4)/data_per_code = ${ADDR}/;" \
                   -e "s/a_sub_b = (true|false)/a_sub_b = false/;" \
                   -e "s/div_bit = [0-9]+/div_bit = 0/;" \
                   -e "s/file_name = [^;]+/file_name = \"$(echo "${FILE}" | sed -E 's/\//\\\//g')\"/;" \
                   -e "s/oob_check = [0-9]+/oob_check = ${CHECK}/;" \
                   -e "s/print_rounds = [0-9]+/print_rounds = ${PRINT}/;" \
                   -e "s/data_t = [^;]+/data_t = int64_t/" "/subleq-mpc/parties_preprocessing/constants.hpp"
    echo "Constants:"
    cat "/subleq-mpc/parties_preprocessing/constants.hpp" | head -n 19 | tail -n 8
    cat "/subleq-mpc/parties_preprocessing/constants.hpp" | head -n 27 | tail -n 1
    cd /subleq-mpc/parties_preprocessing
    make "${PARTY}"
else
    perl -0777 -pi -e "s/data_size = [0-9]+/data_size = ${SIZE}/;" \
                   -e "s/data_per_code = (3|4)/data_per_code = ${ADDR}/;" \
                   -e "s/a_sub_b = (true|false)/a_sub_b = false/;" \
                   -e "s/div_bit = [0-9]+/div_bit = 0/;" \
                   -e "s/file_name = [^;]+/file_name = \"$(echo "${FILE}" | sed -E 's/\//\\\//g')\"/;" \
                   -e "s/oob_check = [0-9]+/oob_check = ${CHECK}/;" \
                   -e "s/max_rounds = -?[0-9]+/max_rounds = -1/;" \
                   -e "s/buf_rounds = [0-9]+/buf_rounds = ${BUF}/;" \
                   -e "s/print_rounds = [0-9]+/print_rounds = ${PRINT}/;" \
                   -e "s/data_t = [^;]+/data_t = int64_t/" "/subleq-mpc/parties_online_only/constants.hpp"
    echo "Constants:"
    cat "/subleq-mpc/parties_online_only/constants.hpp" | head -n 18 | tail -n 9
    cat "/subleq-mpc/parties_online_only/constants.hpp" | head -n 28 | tail -n 1
    cd /subleq-mpc/parties_online_only
    make "${PARTY}"
fi
