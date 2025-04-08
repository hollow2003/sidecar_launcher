#!/bin/bash
cd /home/hpr/edge_sidecarlogger
sudo ./sidecar -service_config "$1" -control_panel "$2" -ntp_address "$3" -redis_address "$4" -redis_port "$5" -target_redis_address "$6" -target_redis_port "$7"&
