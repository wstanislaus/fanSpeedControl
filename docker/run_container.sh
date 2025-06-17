#!/bin/bash

set -e

check_image() {
    echo "Checking if image fan-control-system:latest exists..."
    docker image inspect fan-control-system:latest >/dev/null 2>&1
    return $?
}

check_stopped_container() {
    CONTAINER_ID=$(docker ps -aq -f ancestor=fan-control-system:latest)
    if [ -n "$CONTAINER_ID" ]; then
        CONTAINER_STATUS=$(docker inspect $CONTAINER_ID --format '{{.State.Status}}')
        if [ "$CONTAINER_STATUS" = "exited" ]; then
            echo "Found stopped container $CONTAINER_ID. Removing it..."
            docker rm $CONTAINER_ID
            return 1
        fi
    fi
    return 0
}

build_image() {
    echo "Building Docker image..."
    docker build -t fan-control-system:latest -f Dockerfile --build-arg USER_ID=$(id -u) --build-arg GROUP_ID=$(id -g) ..
}

run_container() {
    # Check if already Container is running for image fan-control-system:latest
    CONTAINER_ID=$(docker ps -q -f ancestor=fan-control-system:latest)
    if [ -n "$CONTAINER_ID" ]; then
        echo "Container $CONTAINER_ID is already running for image fan-control-system:latest"
        # exec to the container
        docker exec -it $CONTAINER_ID /bin/bash
        exit 0
    fi

    echo "Running Docker container..."

    SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

    # Map the source code directory to /host/src/fan_control_system
    SOURCE_DIR="$(realpath "$SCRIPT_DIR/../")"

    INPUT_ARGS=" \
        -v $SOURCE_DIR:/app \
        -e DEBIAN_FRONTEND=noninteractive \
        --name fan-control-container \
        fan-control-system:latest \
        sleep infinity \
        "

    echo "Input args: $INPUT_ARGS"

    CONTAINER_ID=$(docker run -d $INPUT_ARGS)

    echo "Waiting for container $CONTAINER_ID to start..."
    sleep 3

    CONTAINER_STATUS=$(docker inspect $CONTAINER_ID --format '{{.State.Status}}')
    echo "Container Status: $CONTAINER_STATUS"

    if [ "$CONTAINER_STATUS" != "running" ]; then
        echo "Error: Container is not running (Status: $CONTAINER_STATUS)"
        docker logs $CONTAINER_ID
        exit 1
    fi

    echo "building the application..."
    docker exec $CONTAINER_ID bash -c "
        cd /app &&
        sudo mkdir -p /etc/fan_control_system &&
        sudo mkdir -p /var/log/fan_control_system &&
        sudo chmod 777 /var/log/fan_control_system &&
        make clean &&
        make -j$(nproc) &&
        sudo make install
    "

    echo "run mosquitto server in the background..."
    docker exec $CONTAINER_ID bash -c "
        mosquitto -d
    "

    echo "Starting interactive shell..."
    docker exec -it $CONTAINER_ID /bin/bash
}

echo "Checking Docker setup..."
if ! command -v docker &> /dev/null; then
    echo "Error: Docker is not installed"
    exit 1
fi



if ! check_image; then
    echo "Image fan-control-system:latest not found. Building..."
    build_image
fi

check_stopped_container
run_container