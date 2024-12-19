FROM ubuntu:22.04

RUN apt update && apt install -y git g++ cmake python3 iproute2

COPY ./patches /patches
COPY ./container_files /container_files

WORKDIR /
RUN git clone https://github.com/GarbledComputer/GC-Lite.git
RUN /patches/obliv_mult.sh
RUN /patches/step_order.sh
RUN /patches/send_vec.sh
RUN /patches/tcp.sh
RUN /patches/stdout_flush.sh

WORKDIR /GC-Lite/GC-Lite/build
RUN rm -rf ./*
RUN cmake ..
RUN make
