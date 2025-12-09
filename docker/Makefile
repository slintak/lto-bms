# Name of the project
PROJECT_NAME ?= avr-gcc
PROJECT_DOCKER_FINAL ?= ghcr.io/slintak/$(PROJECT_NAME):latest

.PHONY: build push run clean

build:
	docker build --pull -f Dockerfile -t $(PROJECT_DOCKER_FINAL) .

push:  ## Push final image to registry
	docker push $(PROJECT_DOCKER_FINAL)

run:
	docker run --rm -it $(PROJECT_DOCKER_FINAL) /bin/bash

clean:
	docker image prune -f
