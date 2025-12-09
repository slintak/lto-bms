FROM debian:latest

# Install necessary dependencies and AVR-GCC toolchain
RUN apt-get update && apt-get install -y \
    git \
    gcc-avr \
    binutils-avr \
    avr-libc \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /workspace

# Default command
CMD ["bash"]
