#!/bin/bash
if [ $# -eq 1 ]
  then
    # for i in "q" "qosq"; do
    if [ "$1" = "miguel_18" ]
      then
        for j in "miguel_18" "miguel_19" "miguel_20" "miguel_21"; do
        # for j in "miguel_18" ; do
          for i in "q"; do
            echo "./waf --run \"thomasAODV --unit_test_situation --doTest=test_$j.txt --a=$i\""
            ./waf --run "thomasAODV --unit_test_situation --doTest=test_$j.txt --a=$i"
            mv out_stats_"$i"_node18.csv out_stats_"$i"_node18_"$j".csv
          done
        done
    elif [ "$1" = "miguel_19" ]
      then
        echo "18 only, not 19 / 20 / 21 separately please"
    elif [ "$1" = "miguel_20" ]
      then
        echo "18 only, not 19 / 20 / 21 separately please"
    elif [ "$1" = "miguel_21" ]
      then
        echo "18 only, not 19 / 20 / 21 separately please"
    fi
  else
    echo "Please supply a test script to run with aodv, q, qosq."
fi
