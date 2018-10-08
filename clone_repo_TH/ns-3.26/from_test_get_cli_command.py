import os
import sys
import re

def file_iter(file_ptr):
    while True:
        line = file_ptr.readline()
        if not line:
            break
        yield line

if __name__ == "__main__":

    map_cli_to_test_settings = {
        "pcap" : "pcap=",
        "printRoutes" : "printRoutes=",
        "Number of nodes" : "numberOfNodes=",
        "totalTime" : "totalTime=",
        "qlearn" : "no_q",
        "eps" : "eps=",
        "learning_rate" : "learn=",
        "learning_phases" : "learning_phases=",
        "trafficType" : "traffic=",
        "ideal" : "ideal=",
        "qos_qlearning" : "qos_qlearning=",
        "maxpackets": "maxpackets=",
        "metrics_back_to_src":"metrics_back_to_src=",
        "smaller_learning_traffic":"smaller_learning_traffic=",
        "qconv":"q_conv_thresh=",
        "rho":"rho=",
        "max_retry":"max_retry=",
        "learn_more_threshold":"learn_more_threshold=",
    }

    test_name = "QLearningTests/test"+sys.argv[1]+".txt"

    command = "./waf --run \"thomasAODV " + "--doTest="+test_name.split('/')[1]+""

    node_start = re.compile(" *\"node[0-9]*\":{.*")
    test_setting = re.compile(" *\".*\":.*,")
    end_delim = re.compile(" *}..*")

    node_started = False

    for line in file_iter(open(test_name)):
        if (node_start.match(line[:-1]) ):
            node_started = True
        elif (node_started and test_setting.match(line[:-1]) ):
            pass
        elif (end_delim.match(line[:-1]) and node_started):
            node_started = False
        elif (test_setting.match(line[:-1]) ):
            if ("\"NameDesc\"" in line):
                continue
            if (line[:-1].split(':')[0].split('\"')[1] == "qlearn"):
                if (line[:-1].split(':')[1].split(',')[0] == "false"):
                    command = command + " --" + map_cli_to_test_settings[line[:-1].split(':')[0].split('\"')[1] ]
            elif (line[:-1].split(':')[0].split('\"')[1] in map_cli_to_test_settings.keys()):
                command = command + " --" + map_cli_to_test_settings[line[:-1].split(':')[0].split('\"')[1] ] + line[:-1].split(':')[1].split(',')[0]
    command += "\""

    if (len(sys.argv) > 2) :
        if (sys.argv[2] == "gdb"):
            gdb_command = command.split("\"thomasAODV")
            gdb_command = gdb_command[0] + "thomasAODV --command-template=\"gdb --args %s" + gdb_command[1]
            print("")
            print(gdb_command)
        elif (sys.argv[2] == "test"):
            test_command = command.split('--doTest')
            doTest = "--doTest"+test_command[1].split('.txt')[0]+".txt"
            print(test_command[0]+"--unit_test_situation "+doTest+"\"")
    else :
        print("")
        print(command)
