import sys
import argparse
import numpy as np
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
from pylab import savefig

# example commands:
## python jjj_plot_jitter_graph.py -n 18  -p /home/uauser/Desktop/UA/MasterThesis/simulations/test18/eps/ -t lrn_total -j eps,0,0.4,0.7,1.0
## python jjj_plot_jitter_graph.py -n 18  -p /home/uauser/Desktop/UA/MasterThesis/simulations/test18/eps/ -t lrn_total -m ALGOUT -j eps,DELAY,,,

def file_iter(file_ptr):
    while True:
        line = file_ptr.readline()
        if not line:
            break
        yield line

def get_value_from_ALGORITHM_OUTPUT(filename, desired_value):
    ret = 0
    data = open(filename,"r")
    for line in file_iter(data):
        if (desired_value == "TOTAL_LRN_SENT"):
            if ("LRN TRAFFIC" in line and not "QLRN" in line):
                for i in line.split("LRN TRAFFIC:   ")[1].split('/')[:-1]:
                    ret += int(i)
        elif (desired_value == "TOTAL_LRN_RECV"):
            if ("LRN REC TRAFFIC" in line and not "QLRN" in line):
                for i in line.split("LRN REC TRAFFIC:   ")[1].split('/')[:-1]:
                    ret += int(i)
        elif (desired_value == "TOTAL_LRN_RECV_AT_DST"):
            if ("LRN REC TRAFFIC" in line and not "QLRN" in line):
                for i in line.split("LRN REC TRAFFIC:   ")[1].split('/')[:-1]:
                    ret = int(i)
        elif (desired_value == "TOTAL_QLRN_RECV"):
            if ("LRN TRAFFIC" in line and not "QLRN" in line):
                for i in line.split("LRN TRAFFIC:   ")[1].split('/')[:-1]:
                    ret += int(i)
        elif (desired_value == "DATA_SENT"):
            if ("DATA TRAFFIC" in line):
                for i in line.split("DATA TRAFFIC:   ")[1].split('/')[:-1]:
                    ret += int(i)
        elif (desired_value == "DATA_RECV"): #but dont panic its a longer path so this isnt a particularly useful way to look at it
            if ("DATA REC TRAFFIC" in line):
                for i in line.split("DATA REC TRAFFIC:   ")[1].split('/')[:-1]:
                    ret += int(i)
        elif (desired_value == "DELAY"):
            if ("Observed average delay" in line):
                ret = float(line.split("+")[1].split('ms')[0]  )
        else :
            print("cant handle this."+str(desired_value)+" param_v_1 must be among [TOTAL_LRN_SENT |TOTAL_LRN_RECV | TOTAL_LRN_RECV_AT_DST|TOTAL_QLRN_RECV |DATA_SENT |DATA_RECV | DELAY]")
            sys.exit(0)
    return ret

def create_graph_algout(filenames,param,desired_y_value):
    print("Looking for the values for " + desired_y_value + " in test files ")

    x_values = []
    y_values = []

    labels_dict = {
        "TOTAL_LRN_SENT": "Number of learning packets transmitted by source" ,
        "TOTAL_LRN_RECV": "Number of learning packets received in the network",
        "TOTAL_LRN_RECV_AT_DST": "Number of learning packets reaching the destination" ,
        "TOTAL_QLRN_RECV": "Number of Q-Info packets exchanged" ,
        "DATA_SENT": "Total number of data packets transmitted" ,
        "DATA_RECV": "Total number of data packets received" ,
        "DELAY": "Delay(ms)" ,
    }
    y_label = labels_dict[desired_y_value]

    param_values_dict = {
        "epsilon":[0,0.4,0.7,1.0],
        "gamma":[0,0.3,0.6,0.9],
        "rho":[0.25,0.5,0.75,1],
        "ConvThreshold":[0.001,0.01,0.05,0.1],
        "LearnMore":[0.001,0.01,0.1,0.5],
        "learning_rate":[0.25,0.5,0.75,1.0],
        "MaxRetries":[1,4,8,12],
    }

    for i in xrange(len(filenames) ):
        print(desired_y_value)
        y_values.append(get_value_from_ALGORITHM_OUTPUT(filenames[i],desired_y_value))
        x_values.append(param_values_dict[param][i])
    return x_values, y_values, y_label

def create_graph(filename, ignore_learning):
    if (   not filename) :
        print(filename)
        return [],[],[],[],[],[]

    data = open(filename, "r")

    timestamps = []
    lrn_pkt_timestamps = []
    values = []
    skip_1st = True
    nr_of_correct_pkts_seen = 0.0
    nr_of_incorrect_pkts_seen = 0.0
    metric_values = []
    lrn_pkts_per_traff_pkt = []
    lrn_pkts_total = []
    lrn_pkts_seen = 0.0
    for line in file_iter(data):
        metric_delay = "NOK"
        metric_loss = "NOK"
        metric_jitter = "NOK"
        metric_ok = "NOK"

        if (skip_1st):
            # pktID,currTime,delay,initial_estim,learning,metrics,second_to_last_hop,trafficType
            skip_1st = False
            continue
        # pktID,currTime,delay,initial_estim,learning,trafficType,metrics
        pktID , timestamp,delay_value , q_value,learning , metric_delay , metric_jitter , metric_loss , second_to_last_hop , traffictype = line[:-1].split(',')
        # if (learning == "not_learning") :print(second_to_last_hop)
        if ( metric_jitter == "OK" and metric_loss == "OK"):
            metric_ok = "OK"


        if ((ignore_learning and learning == "learning") ):
            total_nr_of_traffic_seen = nr_of_correct_pkts_seen + nr_of_incorrect_pkts_seen
            if (total_nr_of_traffic_seen != 0):
                # print(total_nr_of_traffic_seen,float(timestamp[1:-1]))
                # print(len(lrn_pkts_per_traff_pkt)/total_nr_of_traffic_seen)
                lrn_pkts_per_traff_pkt.append(len(lrn_pkts_per_traff_pkt)/float(total_nr_of_traffic_seen))
            else:
                lrn_pkts_per_traff_pkt.append(0.0)
            # print(lrn_pkts_total,float(timestamp[1:-1]))
            lrn_pkt_timestamps.append(float(timestamp[1:-1]))
            lrn_pkts_seen += 1
            lrn_pkts_total.append(lrn_pkts_seen)
        else:
            # if (float(delay_value[:-2]) > 100) :
            #     print(pktID)
            timestamps.append(float(timestamp[1:-1]))
            values.append(float(delay_value[1:-2])) #TODO nasty, delay value should be positive and in ns...
            # len(metric_values) = 0
            if (metric_ok == "OK"):
                nr_of_correct_pkts_seen += 1.0
            elif (metric_ok == "NOK"):
                nr_of_incorrect_pkts_seen += 1.0
            else :
                print(metric_ok)
            metric_values.append(nr_of_correct_pkts_seen / (nr_of_correct_pkts_seen + nr_of_incorrect_pkts_seen) )
            lrn_pkt_timestamps.append(float(timestamp[1:-1]))
            lrn_pkts_total.append(lrn_pkts_seen)
    return timestamps,values,metric_values,lrn_pkts_per_traff_pkt,lrn_pkt_timestamps,lrn_pkts_total


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-n', '--dst_node', type=str, dest="dest_node_id", required=False, default=None)

    parser.add_argument('-t', '--type_of_graph', type=str, dest="graph_type", required=False, default="delay")
    parser.add_argument('-i', '--ignore_learning', type=bool, dest="ignore_learning", required=False, default=True)

    parser.add_argument('-of', '--output_filename', type=str, dest="output_filename", required=False, default=None)
    parser.add_argument('-p', '--path', type=str, dest="sim_path", required=False, default="")

    parser.add_argument('-q', '--q',type=str, dest="type_of_alg", required=False, default="q")

    parser.add_argument('-j', '--j', type=str, dest="pretty_graph", required=True)
    parser.add_argument('-m', '--m', type=str, dest="mode", required=False, default=None)

    args = parser.parse_args()

    fig, axes = plt.subplots(figsize=(15, 7.5)) #inches for some reason
    index = 0

    param_value = args.pretty_graph.split(',')[0]
    param_v_1 = args.pretty_graph.split(',')[1]
    param_v_2 = args.pretty_graph.split(',')[2]
    param_v_3 = args.pretty_graph.split(',')[3]
    param_v_4 = args.pretty_graph.split(',')[4]

    filenames = ["","","",""]
    if (args.dest_node_id and not args.mode) :
        filenames[0] = args.sim_path+"out_stats_"  + args.type_of_alg + "_node"+args.dest_node_id+"_miguel_18.csv"
        filenames[1] = args.sim_path+"out_stats_"  + args.type_of_alg + "_node"+args.dest_node_id+"_miguel_19.csv"
        filenames[2] = args.sim_path+"out_stats_"  + args.type_of_alg + "_node"+args.dest_node_id+"_miguel_20.csv"
        filenames[3] = args.sim_path+"out_stats_"  + args.type_of_alg + "_node"+args.dest_node_id+"_miguel_21.csv"

        timestamps_miguel_18,values_miguel_18,percent_metric_ok_miguel_18,lrn_pkt_per_traff_18,lrn_pkt_timestamps_18,lrn_total_18 = create_graph(filenames[0], args.ignore_learning)
        timestamps_miguel_19,values_miguel_19,percent_metric_ok_miguel_19,lrn_pkt_per_traff_19,lrn_pkt_timestamps_19,lrn_total_19 = create_graph(filenames[1], args.ignore_learning)
        timestamps_miguel_20,values_miguel_20,percent_metric_ok_miguel_20,lrn_pkt_per_traff_20,lrn_pkt_timestamps_20,lrn_total_20 = create_graph(filenames[2], args.ignore_learning)
        timestamps_miguel_21,values_miguel_21,percent_metric_ok_miguel_21,lrn_pkt_per_traff_21,lrn_pkt_timestamps_21,lrn_total_21 = create_graph(filenames[3], args.ignore_learning)
        axes.set_xlim(left=-10, right=max(timestamps_miguel_18  + timestamps_miguel_19  + timestamps_miguel_20 + timestamps_miguel_21)+10, emit=True, auto=False)
    elif (args.mode == "ALGOUT"):
        filenames[0] = args.sim_path+ args.type_of_alg + "ALGORITHM_OUTPUT_test_miguel_18.txt"
        filenames[1] = args.sim_path+ args.type_of_alg + "ALGORITHM_OUTPUT_test_miguel_19.txt"
        filenames[2] = args.sim_path+ args.type_of_alg + "ALGORITHM_OUTPUT_test_miguel_20.txt"
        filenames[3] = args.sim_path+ args.type_of_alg + "ALGORITHM_OUTPUT_test_miguel_21.txt"
        args.graph_type = "COMPARE"
    else:
        print("invalid mode given (-m [ALGOUT] )")
        sys.exit(0)

    font = {'family' : 'sans-serif',
            'weight' : 'normal',
            'size'   : 18}
    plt.rc('font', **font)
    params = {'legend.fontsize': 'medium',
         'axes.labelsize': 'x-large',
         'xtick.labelsize':'x-large',
         'ytick.labelsize':'x-large'}

    plt.rcParams.update(params)
    for item in ([axes.title, axes.xaxis.label, axes.yaxis.label] +
        axes.get_xticklabels() + axes.get_yticklabels()):
        item.set_fontsize(16)

    if (args.graph_type == "delay"):
        axes.set_ylim(bottom=-1, top=max(values_miguel_18+values_miguel_19+values_miguel_20+values_miguel_21)+10, emit=True, auto=False)
        axes.plot(timestamps_miguel_18, values_miguel_18, color='red')
        axes.plot(timestamps_miguel_19, values_miguel_19, color='purple')
        axes.plot(timestamps_miguel_20, values_miguel_20, color='blue')# linestyle='-.')
        axes.plot(timestamps_miguel_21, values_miguel_21, color='green')
        axes.set_xlabel('Time in simulation (s)')
        axes.set_ylabel('Delay (ms)')
        axes.set_title("Comparing delay for the four values chosen for the parameter "+param_value, x=0.50)
    elif (args.graph_type == "jitter"):
        values_miguel_18 = [abs(float(v2) - float(v1)) for v1,v2 in zip(values_miguel_18,values_miguel_18[1:])]
        values_miguel_19 = [abs(float(v2) - float(v1)) for v1,v2 in zip(values_miguel_19,values_miguel_19[1:])]
        values_miguel_20 = [abs(float(v2) - float(v1)) for v1,v2 in zip(values_miguel_20,values_miguel_20[1:])]
        values_miguel_21 = [abs(float(v2) - float(v1)) for v1,v2 in zip(values_miguel_21,values_miguel_21[1:])]
        axes.set_ylim(bottom=-1, top=max(values_miguel_18+values_miguel_19+values_miguel_20+values_miguel_21)+10, emit=True, auto=False)
        values_miguel_18.insert(0,0)
        values_miguel_19.insert(0,0)
        values_miguel_20.insert(0,0)
        values_miguel_21.insert(0,0)
        axes.plot(timestamps_miguel_18, values_miguel_18)
        axes.plot(timestamps_miguel_19, values_miguel_19)
        axes.plot(timestamps_miguel_20, values_miguel_20)
        axes.plot(timestamps_miguel_21, values_miguel_21)
        axes.set_xlabel('Time in simulation (s)')
        axes.set_ylabel("Packet jitter compared to previous packet (ms)")
        axes.set_title("Comparing jitter for the four values chosen for the parameter "+param_value, x=0.50)
    elif (args.graph_type == "metrics"):
        axes.set_ylim(bottom=-0.1, top=max(percent_metric_ok_miguel_18+percent_metric_ok_miguel_19+percent_metric_ok_miguel_20+percent_metric_ok_miguel_21)+0.1, emit=True, auto=False)
        axes.plot(timestamps_miguel_18, [1]*len(timestamps_miguel_18), color='black',  )
        axes.plot(timestamps_miguel_18, percent_metric_ok_miguel_18, color='red')
        axes.plot(timestamps_miguel_19, percent_metric_ok_miguel_19, color='purple')
        axes.plot(timestamps_miguel_20, percent_metric_ok_miguel_20, color='blue')
        axes.plot(timestamps_miguel_21, percent_metric_ok_miguel_21, color='green')
        axes.set_xlabel('Time in simulation (s)')
        axes.set_ylabel('% correct packets')
        axes.set_title("Comparing packet metrics for the four values chosen for the parameter "+param_value, x=0.50)
    elif (args.graph_type == "lrn_per_traffic"):
        axes.set_ylim(bottom=-0.1, top=max(lrn_pkt_per_traff_18+lrn_pkt_per_traff_19+lrn_pkt_per_traff_20+lrn_pkt_per_traff_21)+0.1, emit=True, auto=False)
        axes.plot(lrn_pkt_timestamps_21, lrn_pkt_per_traff_21, color='red')
        axes.plot(lrn_pkt_timestamps_18, lrn_pkt_per_traff_18, color='purple')
        axes.plot(lrn_pkt_timestamps_19, lrn_pkt_per_traff_19, color='blue')
        axes.plot(lrn_pkt_timestamps_20, lrn_pkt_per_traff_20, color='green')
        axes.set_xlabel('Time in simulation (s)')
        axes.set_ylabel('ratio of learning packets received at dst for every traffic packet received by dst')
        axes.set_title("Comparing packet metrics for the four values chosen for the parameter "+param_value, x=0.50)
    elif (args.graph_type == "lrn_total"):
        axes.set_ylim(bottom=-1, top=max(lrn_total_18+lrn_total_19+lrn_total_20+lrn_total_21)+10, emit=True, auto=False)
        axes.plot(lrn_pkt_timestamps_18, lrn_total_18, color='red')
        axes.plot(lrn_pkt_timestamps_19, lrn_total_19, color='purple')
        axes.plot(lrn_pkt_timestamps_20, lrn_total_20, color='blue')
        axes.plot(lrn_pkt_timestamps_21, lrn_total_21, color='green')
        axes.set_xlabel('Time in simulation (s)')
        axes.set_ylabel('Number of learning packets, total, used')
        axes.set_title("Comparing packet metrics for the four parameter values (param = "+param_value+")", x=0.50)
    elif (args.graph_type == "COMPARE"):
        x_vals,y_vals,y_label = create_graph_algout (filenames,param_value,param_v_1)
        margin = max(y_vals) * 0.05
        margin_x =  max(x_vals) * 0.05
        axes.set_ylim(bottom=-margin, top=max(y_vals)+margin, emit=True, auto=False)
        axes.set_xlim(left=-margin_x, right=max(x_vals)+margin_x, emit=True, auto=False)
        axes.plot(x_vals, y_vals, color='black',marker='o',linestyle='--')
        axes.set_xlabel('Parameter values')
        axes.set_ylabel(y_label)
        axes.set_title("Statistics of the EQ-Routing algorithm with different values for the parameter " + param_value, x=0.50)
    else :
        print("invalid graph type given (-t [delay|jitter|metrics|lrn_per_traffic|lrn_total] )")
        sys.exit(0)
    # Shrink current axis by 20%
    box = axes.get_position()
    # axes.set_position([box.x0, box.y0, box.width * 0.85, box.height])

    # Put a legend to the right of the current axis
    # axes.legend(loc='center left', bbox_to_anchor=(1, 0.5)) # throws a warning ?
    if (args.graph_type == "metrics"):
        axes.set_position([box.x0, box.y0, box.width * 0.85, box.height])
        axes.legend(("IDEAL", ""+param_value+" = "+param_v_1+"", ""+param_value+" = "+param_v_2+"",""+param_value+" = "+param_v_3+"",""+param_value+" = "+param_v_4+""),  shadow=True,loc='center left', bbox_to_anchor=(1, 0.5))
    elif (args.mode != "ALGOUT"):
        axes.set_position([box.x0, box.y0, box.width * 0.85, box.height])
        axes.legend((""+param_value+" = "+param_v_1+"",""+param_value+" = "+param_v_2+"",""+param_value+" = "+param_v_3+"",""+param_value+" = "+param_v_4+""),  shadow=True,loc='center left', bbox_to_anchor=(1, 0.5))
    else:
        # axes.legend((""+param_value+" = "+param_v_1+"",""+param_value+" = "+param_v_2+"",""+param_value+" = "+param_v_3+"",""+param_value+" = "+param_v_4+""),  shadow=True,loc='center left', bbox_to_anchor=(1, 0.5))
        pass

    if(args.output_filename):
        plt.savefig(args.output_filename,bbox_inches='tight')
    else:
        plt.show()
