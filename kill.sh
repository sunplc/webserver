#!/bin/bash

ps aux|grep webserver|grep -v grep|awk '{print $2}'|xargs kill

rm -f webserver.pid

echo > log/access.log
echo > log/server.log

make clean && make

# ./webserver start

