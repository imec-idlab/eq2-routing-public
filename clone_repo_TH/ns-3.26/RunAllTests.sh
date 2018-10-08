#!/bin/bash
if [ $# -eq 1 ]
  then
    if [[ $(($1)) != $1 ]]; then
        echo $1
        ./waf --run "QLrnTests --s"
      else
        ./waf --run "QLrnTests --t=$1"
    fi
  else
    ./waf --run "QLrnTests"
fi
