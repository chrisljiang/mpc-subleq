#!/bin/bash

docker image build --tag testing:gc-lite --force-rm --file Dockerfile .
