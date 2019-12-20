#!/bin/bash
if [ $# -eq 1 ]
  then
    # for i in "A" "B" "C"; do
    for i in "A"; do
      echo "./waf --run \"thomasAODV --unit_test_situation --doTest=test_$1.txt --a=qosq --traffic_miguel_test=voip/traffic$i\""
      ./waf --run "thomasAODV --unit_test_situation --doTest=test_$1.txt --a=qosq --traffic_miguel_test=voip/traffic$i"
      mv out_stats_qosq_node18.csv out_stats_qosq_node18_$i.csv
      mv qosqroute_t1950_test_$1.txt qosqroute_t1950_test_$1_$i.txt
      mv qosqroute_t950_test_$1.txt qosqroute_t950_test_$1_$i.txt
    done
  else
    echo "Please supply a test script to run with aodv, q, qosq."
fi
