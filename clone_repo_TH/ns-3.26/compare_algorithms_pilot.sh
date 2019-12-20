#!/bin/bash
if [ $# -eq 1 ]
  then
    # for i in "q" "aodv" "qosq"; do
    for i in "q"; do
      echo "./waf --run \"thomasAODV --unit_test_situation --doTest=test_$1.txt --a=$i\""
      ./waf --run "thomasAODV --unit_test_situation --doTest=test_$1.txt --a=$i"
    done
  else
    echo "Please supply a test script to run with aodv, q, qosq."
fi
