#!/bin/bash

perl -0777 -pi -e 's/double ObliviousMultiplication/long ObliviousMultiplication/' /GC-Lite/GC-Lite/include/alice.h
perl -0777 -pi -e 's/double ObliviousMultiplication/long ObliviousMultiplication/' /GC-Lite/GC-Lite/include/bob.h

perl -0777 -pi -e 's/double Alice::ObliviousMultiplication/long Alice::ObliviousMultiplication/;' \
               -e 's/double result/long result/;' \
               -e 's/msg_for_Bob\.scalar1 = arg1.*\n.*/msg_for_Bob.scalar1 = arg1 + Randoms[rc];\n    msg_for_Bob.scalar2 = arg2 + Randoms[rc + 2];/;' \
               -e 's/result \+= \(\(arg1.*\n.*\n.*/result = arg1*(arg2+msg_from_Bob.scalar2) - Randoms[rc+2]*(msg_from_Bob.scalar1) + Randoms[rc+3];/;' \
               -e 's/double next_pc/long next_pc/;' \
               -e 's/double temp/long temp/;' \
               -e 's/double result = 0;/long result = 0;/' \
               /GC-Lite/GC-Lite/src/alice.cpp
perl -0777 -pi -e 's/double Bob::ObliviousMultiplication/long Bob::ObliviousMultiplication/;' \
               -e 's/double result/long result/;' \
               -e 's/msg_for_Alice\.scalar1 = arg2.*\n.*/msg_for_Alice.scalar1 = arg1 + Randoms[rc];\n    msg_for_Alice.scalar2 = arg2 + Randoms[rc + 2];/;' \
               -e 's/result \+= \(\(arg2.*\n.*\n.*/result = arg1*(arg2+msg_from_Alice.scalar2) - Randoms[rc+2]*(msg_from_Alice.scalar1) + Randoms[rc+3];/;' \
               -e 's/double next_pc/long next_pc/;' \
               -e 's/double temp/long temp/;' \
               -e 's/double result = 0;/long result = 0;/' \
               /GC-Lite/GC-Lite/src/bob.cpp

perl -0777 -pi -e 's/ {8}fout << R1\[i\] << " ";\n {8}fout << R1\[i\] \* R2\[i\] \+ R\[i\] << "\\n";/        fout << R1[i] << " ";\n        if (i % 2 == 1)\n        {\n            fout << R1[i] * R2[i-1] + R[i] << "\\n";\n        }\n        else\n        {\n            fout << 0 << "\\n";\n        }/' /GC-Lite/GC-Lite/src/helper.cpp
perl -0777 -pi -e 's/ {8}fout << R2\[i\] << " ";\n {8}fout << R1\[i\] \* R2\[i\] - R\[i\] << "\\n";/        fout << R2[i] << " ";\n        if (i % 2 == 1)\n        {\n            fout << R1[i-1] * R2[i] - R[i] << "\\n";\n        }\n        else\n        {\n            fout << 0 << "\\n";\n        }/' /GC-Lite/GC-Lite/src/helper.cpp
