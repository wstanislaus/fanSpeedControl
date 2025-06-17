#!/bin/bash

# Stop containers using fan-control-system:latest image
echo "Stopping containers using fan-control-system:latest image..."
docker stop $(docker ps -q --filter ancestor=fan-control-system:latest) 2>/dev/null || true

# Remove containers using fan-control-system:latest image
echo "Removing containers using fan-control-system:latest image..."
docker rm $(docker ps -aq --filter ancestor=fan-control-system:latest) 2>/dev/null || true

# Remove the fan-control-system image
# echo "Removing fan-control-system image..."
# docker rmi fan-control-system:latest 2>/dev/null || true

echo "Cleanup complete!" 