# --- Stage 1: Builder ---
FROM ubuntu:22.04 as builder

RUN DEBIAN_FRONTEND="noninteractive" apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt install -y cmake \
clang-13 llvm-13 nasm ninja-build pkg-config \
libcap-dev libglfw3-dev libepoxy-dev python3-dev libsdl2-dev \
python3 linux-headers-generic  \
git  qtbase5-dev qtdeclarative5-dev lld

RUN git clone --recurse-submodules https://github.com/FEX-Emu/FEX.git
WORKDIR /FEX
RUN mkdir build

ARG CC=clang-13
ARG CXX=clang++-13
RUN cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DUSE_LINKER=lld -DENABLE_LTO=True -DBUILD_TESTS=False -DENABLE_ASSERTIONS=False -G Ninja .
RUN ninja

WORKDIR /FEX/build

# --- Stage 2: Runner ---
FROM builder as runner

RUN DEBIAN_FRONTEND="noninteractive" apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt install -y \
libcap-dev libglfw3-dev libepoxy-dev

COPY --from=builder /FEX/Bin/* /usr/bin/

WORKDIR /
