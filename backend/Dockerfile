FROM ubuntu:24.04 AS drogon-builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    git \
    libbrotli-dev \
    libc-ares-dev \
    libhiredis-dev \
    libjsoncpp-dev \
    libpq-dev \
    libsqlite3-dev \
    libssl-dev \
    libyaml-cpp-dev \
    ninja-build \
    pkg-config \
    uuid-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp
RUN git clone --depth 1 --branch v1.9.12 --recurse-submodules https://github.com/drogonframework/drogon.git
WORKDIR /tmp/drogon
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF -DBUILD_ORM=ON \
    && cmake --build build \
    && cmake --install build

FROM ubuntu:24.04 AS app-builder


RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    libbrotli-dev \
    libc-ares-dev \
    libhiredis-dev \
    libjsoncpp-dev \
    libpq-dev \
    libsqlite3-dev \
    libssl-dev \
    libyaml-cpp-dev \
    ninja-build \
    pkg-config \
    uuid-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

COPY --from=drogon-builder /usr/local /usr/local

WORKDIR /app
COPY CMakeLists.txt config.docker.json main.cc ./
COPY controllers ./controllers
COPY services ./services
COPY constants ./constants
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build

FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libbrotli1 \
    libc-ares2 \
    libhiredis1.1.0 \
    libjsoncpp25 \
    libpq5 \
    libsqlite3-0 \
    libssl3t64 \
    libyaml-cpp0.8 \
    wget \
    zlib1g \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=drogon-builder /usr/local /usr/local
COPY --from=app-builder /app/build/YearReporter /app/YearReporter
COPY config.docker.json /app/config.docker.json

EXPOSE 8080

CMD ["./YearReporter", "config.docker.json"]
