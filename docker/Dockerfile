FROM debian:latest

# Install necessary dependencies and AVR-GCC toolchain
RUN apt-get update && apt-get install -y \
    git \
    gcc-avr \
    binutils-avr \
    avr-libc \
    make \
    wget \
    unzip \
    patch \
    && rm -rf /var/lib/apt/lists/*

# Download and install ATTiny series-2 support
RUN wget -O /tmp/attiny.atpack http://packs.download.atmel.com/Atmel.ATtiny_DFP.2.0.368.atpack \
    && unzip /tmp/attiny.atpack -d /tmp/attiny \
    && cp /tmp/attiny/include/avr/iotn?*2[2467].h /usr/lib/avr/include/avr/ \
    && ls /tmp/attiny/gcc/dev/attiny?*2[2467]/avrxmega3/ \
    && cp /tmp/attiny/gcc/dev/attiny?*2[2467]/avrxmega3/short-calls/* /usr/lib/avr/lib/avrxmega3/short-calls/ \
    && cp /tmp/attiny/gcc/dev/attiny?*2[2467]/device-specs/specs-* /usr/lib/gcc/avr/5.4.0/device-specs/ \
    && rm -rf /tmp/attiny.atpack /tmp/attiny

# Apply patch to io.h
COPY io.h.patch /tmp/io.h.patch
RUN patch /usr/lib/avr/include/avr/io.h < /tmp/io.h.patch && rm /tmp/io.h.patch

# Set the working directory
WORKDIR /workspace

# Default command
CMD ["bash"]
