#!/bin/bash

# change working directory to location of script
cd "$(dirname "${0}")"

docker image build --tag testing:subleq-mpc --force-rm --file Dockerfile ..
