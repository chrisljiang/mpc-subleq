#!/bin/bash

perl -0777 -pi -e 's/#include "dataelement\.h"/#include "dataelement.h"\n#include <netinet\/tcp.h>/' /GC-Lite/GC-Lite/include/client.h
perl -0777 -pi -e 's/#include "dataelement\.h"/#include "dataelement.h"\n#include <netinet\/tcp.h>/' /GC-Lite/GC-Lite/include/server.h

perl -0777 -pi -e 's/ +printf\("ERROR connecting %s to Server!\\n", name.c_str\(\)\);/        printf("ERROR connecting %s to Server!\\n", name.c_str());\n    int enable = 1;\n    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0)\n        printf("setsockopt(TCP_NODELAY) failed");\n    if (setsockopt(sockfd, IPPROTO_TCP, TCP_QUICKACK, &enable, sizeof(int)) < 0)\n        printf("setsockopt(TCP_QUICKACK) failed");/' /GC-Lite/GC-Lite/src/client.cpp
perl -0777 -pi -e 's/ +return 0;/    if (setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0)\n        printf("setsockopt(TCP_NODELAY) failed");\n    if (setsockopt(newsockfd, IPPROTO_TCP, TCP_QUICKACK, &enable, sizeof(int)) < 0)\n        printf("setsockopt(TCP_QUICKACK) failed");\n\n    return 0;/' /GC-Lite/GC-Lite/src/server.cpp
