#!/bin/bash

perl -0777 -pi -e 's/ {8}ObliviousExecute\(\);/        std::cout << std::flush;\n        ObliviousExecute();/' /GC-Lite/GC-Lite/src/alice.cpp
perl -0777 -pi -e 's/ {8}ObliviousExecute\(\);/        std::cout << std::flush;\n        ObliviousExecute();/' /GC-Lite/GC-Lite/src/bob.cpp
