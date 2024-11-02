#!/bin/bash
cd /home/hpr/consul/sidecar
sudo ./sidecar -service_config "$1" -control_panel "$2" -ntp_address "$3" -redis_address "$4" -redis_port "$5"&
