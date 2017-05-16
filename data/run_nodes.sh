#!/bin/bash

START_PORT=7000
CLIENT_BIN=../examples/.libs/lt-client_test
TORRENT_FILE_FILES=./bootstrap_node*.torrent
TORRENT_EXTENTION=.torrent
PS=/bin/ps
GREP=/bin/grep
AWK=/usr/bin/awk
KILL=/bin/kill
SLEEP=/bin/sleep
NULL=/dev/null

if [ $1 = "-t" ];
then
    bootstrap_task_pids=`$PS aux | $GREP "bootstrap_node" | $GREP -v grep | $AWK '{ print $2 }'`

    for pid in $bootstrap_task_pids
    do
      $KILL -9 $pid
    done

else

  binding_port=$START_PORT

  for f in $TORRENT_FILE_FILES
  do
    echo "Processing $f file..."
    binding_port=$((binding_port+1))
    $CLIENT_BIN $f -p $binding_port < $NULL&
    $SLEEP 2
  done

fi

exit 0
