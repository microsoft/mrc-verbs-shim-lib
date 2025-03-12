#!/bin/bash

LD_PRELOAD=libverbs_mrc.so ./tests/check_ibv_overwrites
