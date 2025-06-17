# Use Ubuntu 22.04 as the base image
FROM ubuntu:22.04

# Accept user and group IDs as build arguments
ARG USER_ID
ARG GROUP_ID

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build essentials and required packages
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libmosquitto-dev \
    mosquitto \
    mosquitto-clients \
    libyaml-cpp-dev \
    sudo \
    net-tools \
    python3 \
    python3-pip \
    python3-venv \
    protobuf-compiler \
    libprotobuf-dev \
    libgrpc++-dev \
    protobuf-compiler-grpc \
    && rm -rf /var/lib/apt/lists/*

# Create a group with the specified group ID
RUN groupadd -g ${GROUP_ID} developer

# Create a user with the specified user ID and group ID
RUN useradd -u ${USER_ID} -g ${GROUP_ID} -m -s /bin/bash developer

# Add developer to sudo group and configure sudo access
RUN usermod -aG sudo developer && \
    echo "developer ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Set the working directory
WORKDIR /app

# Set permissions for the app directory
RUN chown -R developer:developer /app

# Switch to non-root user
USER developer

# Set the default command to bash
CMD ["/bin/bash"] 