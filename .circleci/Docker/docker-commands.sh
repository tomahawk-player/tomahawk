#!/bin/bash

get_repository_root(){
    local REPOSITORY_ROOT="$(git rev-parse --show-toplevel)"
    echo "$REPOSITORY_ROOT"
}

get_repository_subdir(){
    REPOSITORY_ROOT=$(get_repository_root)
    CUR_DIR=$(pwd)
    SUB_DIR=$(echo "$CUR_DIR" | grep -oP "^$REPOSITORY_ROOT\K.*")
    echo "$SUB_DIR"
}

docker-interactive() {
    REPOSITORY_ROOT=$(get_repository_root)
    SUB_DIR=$(get_repository_subdir)

    echo "SUB_DIR: " "$SUB_DIR"

    source $REPOSITORY_ROOT/.circleci/Docker/Docker.variables
    echo "Starting container: " "$DOCKER_IMAGE_NAME:$DOCKER_IMAGE_VER"

    echo "Got command: " "$*"
    USER_ID=$(id -u $USER)
    echo "Using USER_ID:" $USER_ID

    docker run  --env LOCAL_USER_ID=$USER_ID \
                --rm \
                --interactive \
                --volume $REPOSITORY_ROOT:/tmp/workspace \
                --workdir /tmp/workspace$SUB_DIR \
                --env "TERM=xterm-256color" \
                --tty \
                "$DOCKER_IMAGE_NAME:$DOCKER_IMAGE_VER" /bin/bash
}

docker-help() {
    echo "docker-interactive     - Start an interactive bash session inside docker container and remove it on exit"
    echo "docker-help            - Show this help text"
}
