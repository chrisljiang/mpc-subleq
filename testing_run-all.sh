#!/bin/bash

# change working directory to location of script
cd "$(dirname "${0}")"

# no internet tests
#                                      subleq data| gc data sz| gc prog sz|  instructs|rep
#                                        F   I   L|  F   I   L|  F   I   L|  F   I   L|
./testing_generate-data_no-internet.sh  50   1  50  10   1  10  10   1  10  10   5 150  50  # iterations
./testing_generate-data_no-internet.sh  10   5 500  10   1  10  10   1  10  50   1  50  50  # data size (subleq)
./testing_generate-data_no-internet.sh  50   1  50   2   5 492   2   1   2  50   1  50  50  # data size (gc-lite with small prog)
./testing_generate-data_no-internet.sh  50   1  50   2   5 297  50   1  50  50   1  50  50  # data size (gc-lite with large prog)
./testing_generate-data_no-internet.sh  50   1  50   2   1   2   2   1 124  50   1  50  50  # prog size (gc-lite with small data)
./testing_generate-data_no-internet.sh  50   1  50 200   1 200   2   1  75  50   1  50  50  # prog size (gc-lite with large data)

# simulated internet tests
#                            bandwidth|    latency|subleq data| gc data sz| gc prog sz|  instructs|rep
#                            F   I   L|  F   I   L|  F   I   L|  F   I   L|  F   I   L|  F   I   L|
./testing_generate-data.sh   5   5  50   0 1.0   0  50   1  50  10   1  10  10   1  10  50   1  50  25  # low bandwidth (no latency)
./testing_generate-data.sh  50  25 100   0 1.0   0  50   1  50  10   1  10  10   1  10  50   1  50  25  # mid bandwidth (no latency)
./testing_generate-data.sh 100 100 500   0 1.0   0  50   1  50  10   1  10  10   1  10  50   1  50  25  # high bandwidth (no latency)
./testing_generate-data.sh   5   5  50   5 1.0   5  50   1  50  10   1  10  10   1  10  50   1  50  25  # low bandwidth (high latency)
./testing_generate-data.sh  50  25 100   5 1.0   5  50   1  50  10   1  10  10   1  10  50   1  50  25  # mid bandwidth (high latency)
./testing_generate-data.sh 100 100 500   5 1.0   5  50   1  50  10   1  10  10   1  10  50   1  50  25  # high bandwidth (high latency)
./testing_generate-data.sh   5   1   5   0 0.1   2  50   1  50  10   1  10  10   1  10  50   1  50  25  # low latency (low bandwidth)
./testing_generate-data.sh   5   1   5   2 1.0   5  50   1  50  10   1  10  10   1  10  50   1  50  25  # mid latency (low bandwidth)
./testing_generate-data.sh   5   1   5   5 2.5  15  50   1  50  10   1  10  10   1  10  50   1  50  25  # high latency (low bandwidth)
./testing_generate-data.sh  -1   1  -1   0 0.1   2  50   1  50  10   1  10  10   1  10  50   1  50  25  # low latency (unlimited bandwidth)
./testing_generate-data.sh  -1   1  -1   2 1.0   5  50   1  50  10   1  10  10   1  10  50   1  50  25  # mid latency (unlimited bandwidth)
./testing_generate-data.sh  -1   1  -1   5 2.5  15  50   1  50  10   1  10  10   1  10  50   1  50  25  # high latency (unlimited bandwidth)
