# Name of the project
PROJECT_NAME ?= avr-gcc
PROJECT_DOCKER_FINAL ?= vlastik/$(PROJECT_NAME):latest

.PHONY: build push

build:
	docker build --pull -f Dockerfile -t $(PROJECT_DOCKER_FINAL) .

push:  ## Push final image to registry
	docker push $(PROJECT_DOCKER_FINAL)