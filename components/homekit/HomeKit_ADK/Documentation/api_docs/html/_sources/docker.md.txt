Docker with ADK
===============

## What is Docker

Docker makes use of packaged environments called containers that provide everything necessary to build or run
applications without relying on what is currently installed on the host. You can easily share containers while you work,
and be sure that everyone you share with gets the same container that works in the same way. Please go through
[Docker overview](https://docs.docker.com/get-started/overview/) to get more information.

## How Does ADK use Docker

ADK uses Docker containers to support platforms such as Linux and Raspberry Pi. Following are some of the reasons that
ADK uses Docker containers:
- Docker helps with setting up the development environment without polluting the host.
- Docker guarantees that the same development environment is used irrespective of what host OS is being used for
ADK development.
- Speeds up the development process by using a cross-compiler to compile for Raspberry Pi without having to have a
Raspberry Pi device connected to the host machine at all times.

## How to Install Docker
Please follow instructions at [Get Docker](https://docs.docker.com/get-docker/) for your host OS to install Docker.

## How to Remove Docker

To remove all unused containers, networks, images (both dangling and unreferenced), and optionally, volumes.

```sh
docker stop `docker ps -qa`
docker system prune --volumes --all
```

It is safe at this time to delete the docker application from your host.
