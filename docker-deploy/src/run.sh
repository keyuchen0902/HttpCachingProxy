#!/bin/bash
make clean
make
echo 'begin running proxy server...'
./proxy &
while true ; do continue ; done
