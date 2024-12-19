SOURCEDIR ?= ..
HEADERS := $(shell find $(SOURCEDIR) -name '*.hpp')
OPTIONS := --std=c++17 -march=native -O0 -g -pthread -Wno-ignored-attributes
INCLUDES := -I../libdpf/include -I../libdpf/thirdparty -I../libdpf/thirdparty/asio/asio/include -I../subleq
LIBRARIES := -lbsd -lgmp

all: p0 p1 p2

p0: peer.cpp $(HEADERS)
	g++ $(OPTIONS) $(INCLUDES) -o p0 -DPARTY=\"0\" peer.cpp $(LIBRARIES)

p1: peer.cpp $(HEADERS)
	g++ $(OPTIONS) $(INCLUDES) -o p1 -DPARTY=\"1\" peer.cpp $(LIBRARIES)

p2: p2.cpp $(HEADERS)
	g++ $(OPTIONS) $(INCLUDES) -o p2 p2.cpp $(LIBRARIES)
