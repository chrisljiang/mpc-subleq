#!/bin/bash

perl -0777 -pi -e 's/ObliviousExecute\(\);\n +ObliviousShuffle\(\);\n +ObliviousTranslate\(\);/ObliviousShuffle();\n        ObliviousTranslate();\n        ObliviousExecute();/' /GC-Lite/GC-Lite/src/alice.cpp
perl -0777 -pi -e 's/ObliviousExecute\(\);\n +ObliviousShuffle\(\);\n +ObliviousTranslate\(\);/ObliviousShuffle();\n        ObliviousTranslate();\n        ObliviousExecute();/' /GC-Lite/GC-Lite/src/bob.cpp
