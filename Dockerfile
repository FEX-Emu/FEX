# --- Stage 1: Builder ---
FROM ubuntu:20.04 as builder

RUN DEBIAN_FRONTEND="noninteractive" apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt install -y cmake \
clang-10 llvm-10 nasm ninja-build \
libcap-dev libglfw3-dev libepoxy-dev python3-dev \
python3 linux-headers-generic

COPY . /opt/FEX

CMD [ "mkdir /opt/FEX/build" ]

WORKDIR /opt/FEX/build

ARG CC=clang-10
ARG CXX=clang++-10
RUN cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release
RUN ninja

# --- Stage 2: Runner ---
FROM ubuntu:20.04

RUN DEBIAN_FRONTEND="noninteractive" apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt install -y \
libcap-dev libglfw3-dev libepoxy-dev

COPY --from=builder /opt/FEX/build/Bin/* /usr/bin/

WORKDIR /root
