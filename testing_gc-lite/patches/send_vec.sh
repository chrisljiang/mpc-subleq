#!/bin/bash

perl -0777 -pi -e 's/i < size/i < 1/g;' \
               -e 's/sizeof\(V\[i\]\)/sizeof(V[i])*size/g' \
               /GC-Lite/GC-Lite/src/client.cpp
perl -0777 -pi -e 's/i < size/i < 1/g;' \
               -e 's/sizeof\(V\[i\]\)/sizeof(V[i])*size/g' \
               /GC-Lite/GC-Lite/src/server.cpp
