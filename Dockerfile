# Use ubuntu:latest as base for builder stage image
FROM ubuntu:latest as builder

# Set Monero branch/tag to be used for monerod compilation
ARG MONERO_BRANCH=v0.18.3.4

# Added DEBIAN_FRONTEND=noninteractive to workaround tzdata prompt on installation
ENV DEBIAN_FRONTEND="noninteractive"

# Install dependencies for monerod and xmrblocks compilation
RUN apt-get update \
    && apt-get upgrade -y \
    && apt-get install -y --no-install-recommends \
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
    && apt clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Set compilation environment variables
ENV CFLAGS='-fPIC'
ENV CXXFLAGS='-fPIC'
ENV USE_SINGLE_BUILDDIR=1
ENV BOOST_DEBUG=1

WORKDIR /root

# Clone and compile monerod with all available threads
ARG NPROC
RUN git clone --recursive --branch ${MONERO_BRANCH} \
    --depth 1 --shallow-submodules https://github.com/monero-project/monero.git \
    && cd monero \
    && test -z "$NPROC" && nproc > /nproc || echo -n "$NPROC" > /nproc && make -j"$(cat /nproc)"

# Copy and cmake/make xmrblocks with all available threads
COPY . /root/onion-monero-blockchain-explorer/
WORKDIR /root/onion-monero-blockchain-explorer/build
RUN cmake .. && make -j"$(cat /nproc)"

# Use ldd and awk to bundle up dynamic libraries for the final image
RUN zip /lib.zip $(ldd xmrblocks | grep -E '/[^\ ]*' -o)

# Use ubuntu:latest as base for final image
FROM ubuntu:latest AS final

# Added DEBIAN_FRONTEND=noninteractive to workaround tzdata prompt on installation
ENV DEBIAN_FRONTEND="noninteractive"

# Update Ubuntu packages and install unzip to handle bundled libs from builder stage
RUN apt-get update \
    && apt-get upgrade -y \
    && apt-get install -y --no-install-recommends unzip \
    && apt clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

COPY --from=builder /lib.zip .
RUN unzip -o lib.zip \
    && rm -rf lib.zip \
    && apt purge -y unzip

# Add user and setup directories for monerod and xmrblocks
RUN touch /var/mail/ubuntu && chown ubuntu /var/mail/ubuntu && userdel -r ubuntu \
    && useradd -ms /bin/bash monero \
    && mkdir -p /home/monero/.bitmonero \
    && chown -R monero:monero /home/monero/.bitmonero
USER monero

# Switch to home directory and install newly built xmrblocks binary
WORKDIR /home/monero
COPY --chown=monero:monero --from=builder /root/onion-monero-blockchain-explorer/build/xmrblocks .
COPY --chown=monero:monero --from=builder /root/onion-monero-blockchain-explorer/build/templates ./templates/

# Expose volume used for lmdb access by xmrblocks
VOLUME /home/monero/.bitmonero

# Expose default explorer http port
EXPOSE 8081

ENTRYPOINT ["/bin/sh", "-c"]

# Set sane defaults that are overridden if the user passes any commands
CMD ["./xmrblocks --enable-json-api --enable-autorefresh-option  --enable-pusher"]
