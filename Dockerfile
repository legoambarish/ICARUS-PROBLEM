FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    gdb \
    valgrind \
    python3 \
    python3-pip \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /obc
COPY . /obc

# Rebuild the physics library from portable C source so it is native to the
# build platform (required for multi-arch: the repo ships an x86-64 .a, which
# cannot link on arm64).
RUN gcc -O0 -std=c11 -c lib/obc_physics_build.c -o lib/obc_physics_build.o \
    && ar rcs lib/libobc_physics.a lib/obc_physics_build.o \
    && rm -f lib/obc_physics_build.o

CMD ["/bin/bash"]
