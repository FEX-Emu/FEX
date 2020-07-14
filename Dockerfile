# --- Stage 1: Builder ---
FROM ubuntu:20.04 as builder

RUN DEBIAN_FRONTEND="noninteractive" apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt install -y cmake \
libboost-dev clang-10 llvm-10 nasm ninja-build libnuma-dev \
libcap-dev libglfw3-dev libepoxy-dev

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
RUN DEBIAN_FRONTEND="noninteractive" apt install -y libboost-dev \
libnuma-dev libcap-dev libglfw3-dev libepoxy-dev

COPY --from=builder /opt/FEX/build/Bin/* /usr/bin/
COPY --from=builder /opt/FEX/build/External/SonicUtils/libSonicUtils.so /usr/lib/

WORKDIR /root
