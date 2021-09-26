FROM ubuntu:20.04 as builder

ENV DEBIAN_FRONTEND="noninteractive"

RUN apt-get update && apt install -y --no-install-recommends \
    git \
    build-essential \
    cmake \
    miniupnpc \
    graphviz \
    doxygen \
    pkg-config \
    ca-certificates \
    zip \
    libboost-all-dev \
    libunbound-dev \
    libunwind8-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    libgtest-dev \
    libreadline-dev \
    libzmq3-dev \
    libsodium-dev \
    libhidapi-dev \
    libhidapi-libusb0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /root

RUN git clone --recursive -b release-v0.17 https://github.com/monero-project/monero.git \
    && cd monero \
    && USE_SINGLE_BUILDDIR=1 make

COPY . /root/onion-monero-blockchain-explorer/
WORKDIR /root/onion-monero-blockchain-explorer/build
RUN cmake ..
RUN make

# use ldd and awk to bundle up dynamic libraries for the final image
RUN zip /lib.zip $(ldd xmrblocks | grep -E '/[^\ ]*' -o)

FROM ubuntu:20.04

ENV DEBIAN_FRONTEND="noninteractive"
RUN apt-get update && apt-get install -y --no-install-recommends \
    unzip \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /lib.zip .
RUN unzip -o lib.zip && rm -rf lib.zip

RUN useradd -ms /bin/bash monero \
    && mkdir -p /home/monero/.bitmonero \
    && chown -R monero:monero /home/monero/.bitmonero
USER monero
WORKDIR /home/monero

COPY --chown=monero:monero --from=builder /root/onion-monero-blockchain-explorer/build/xmrblocks .
COPY --chown=monero:monero --from=builder /root/onion-monero-blockchain-explorer/build/templates ./templates/

VOLUME /home/monero/.bitmonero
EXPOSE 8081

ENTRYPOINT ["/bin/sh", "-c", "./xmrblocks"]
