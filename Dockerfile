FROM ubuntu:20.04

RUN DEBIAN_FRONTEND="noninteractive" apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt install -y cmake \
libboost-dev clang-10 llvm-10 nasm ninja-build libnuma-dev \
libcap-dev libglfw3-dev libepoxy-dev

COPY . /opt/FEX

CMD [ "mkdir /opt/FEX/build" ]

WORKDIR /opt/FEX/build

ARG CC=clang-10
ARG CXX=clang++-10
RUN cmake -G Ninja ..
RUN ninja
RUN echo "PATH=\$PATH:/opt/FEX/build/Bin" >> ~/.bashrc

WORKDIR /root
