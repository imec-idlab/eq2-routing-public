import sys
import argparse
import numpy as np
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
from pylab import savefig

def file_iter(file_ptr):
    while True:
        line = file_ptr.readline()
        if not line:
            break
        yield line

def create_graph(filename, ignore_learning):
    if (   not filename) :
        print(filename)
        return [],[],[]

    data = open(filename, "r")

    timestamps = []
    values = []
    skip_1st = True
    nr_of_correct_pkts_seen = 0.0
    nr_of_incorrect_pkts_seen = 0.0
    metric_values = []

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
        # if ((ignore_learning and learning == "learning") or second_to_last_hop == "10.1.1.6"):
            continue
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

    return timestamps,values,metric_values


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-n', '--dst_node', type=str, dest="dest_node_id", required=False, default=None)

    parser.add_argument('-t', '--type_of_graph', type=str, dest="graph_type", required=False, default="delay")
    parser.add_argument('-i', '--ignore_learning', type=bool, dest="ignore_learning", required=False, default=True)

    parser.add_argument('-of', '--output_filename', type=str, dest="output_filename", required=False, default=None)
    parser.add_argument('-p', '--path', type=str, dest="sim_path", required=False, default="")

    args = parser.parse_args()

    fig, axes = plt.subplots(figsize=(15, 7.5)) #inches for some reason
    index = 0

    font = {'family' : 'sans-serif',
            'weight' : 'normal',
            'size'   : 18}
    plt.rc('font', **font)
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

    filenames = ["","",""]
    if (args.dest_node_id) :
        filenames[0] = args.sim_path+"out_stats_qosq_node"+args.dest_node_id+"_A.csv"
        filenames[1] = args.sim_path+"out_stats_qosq_node"+args.dest_node_id+"_B.csv"
        # filenames[1] = "out_stats_qosq_node"+args.dest_node_id+".csv"
        filenames[2] = args.sim_path+"out_stats_qosq_node"+args.dest_node_id+"_C.csv"
    #1st pkt in AODV is RREP fyi
    timestamps_qosq_traffA,values_traffA,percent_metric_ok_traffA = create_graph(filenames[0], args.ignore_learning)
    timestamps_qosq_traffB,values_traffB,percent_metric_ok_traffB = create_graph(filenames[1], args.ignore_learning)
    timestamps_qosq_traffC,values_traffC,percent_metric_ok_traffC = create_graph(filenames[2], args.ignore_learning)

    axes.set_xlim(left=-10, right=max(timestamps_qosq_traffA + timestamps_qosq_traffB + timestamps_qosq_traffC)+10, emit=True, auto=False)

    if (args.graph_type == "delay"):
        axes.set_ylim(bottom=-1, top=max(values_traffA[1:]+values_traffB+values_traffC)+10, emit=True, auto=False)
        # axes.plot(timestamps_qosq_traffA, values_traffA, color='red')
        # axes.plot(timestamps_qosq_traffB, values_traffB, color='blue')# linestyle='-.')
        # axes.plot(timestamps_qosq_traffC, values_traffC, color='green')
        axes.plot(timestamps_qosq_traffA, values_traffA, color='0.0', linewidth = 1.5)
        axes.plot(timestamps_qosq_traffB, values_traffB, color='0.4', linewidth = 1.5)# linestyle='-.')
        axes.plot(timestamps_qosq_traffC, values_traffC, color='0.75', linewidth = 1.5)

        index_a = 500
        index_b = 500
        index_c = 500

        axes.annotate('Traffic A',xy=(timestamps_qosq_traffA[index_a],values_traffA[index_a]), xytext=(timestamps_qosq_traffA[index_a]+200,values_traffA[index_a]+25),
            arrowprops=dict(facecolor='white', shrink=0))
        axes.annotate('Traffic A',xy=(timestamps_qosq_traffA[index_a+3000],values_traffA[index_a+3000]), xytext=(timestamps_qosq_traffA[index_a+3000]-200,values_traffA[index_a+3000]+50),
            arrowprops=dict(facecolor='white', shrink=0))
        axes.annotate('Traffic B',xy=(timestamps_qosq_traffB[index_b],values_traffB[index_b]), xytext=(timestamps_qosq_traffB[index_b]+10,values_traffB[index_b]+70),
            arrowprops=dict(facecolor='white', shrink=0))
        axes.annotate('Traffic B',xy=(timestamps_qosq_traffB[index_b+2500],values_traffB[index_b+2500]), xytext=(timestamps_qosq_traffB[index_b+2500]-250,values_traffB[index_b+2500]+15),
            arrowprops=dict(facecolor='white', shrink=0))
        axes.annotate('Traffic C',xy=(timestamps_qosq_traffC[index_c],values_traffC[index_c]), xytext=(timestamps_qosq_traffC[index_c]+600,values_traffC[index_c]+150),
            arrowprops=dict(facecolor='white', shrink=0))
        axes.annotate('Traffic C',xy=(timestamps_qosq_traffC[index_c+3000],values_traffC[index_c+3000]), xytext=(timestamps_qosq_traffC[index_c]+600,values_traffC[index_c]+150),
            arrowprops=dict(facecolor='white', shrink=0))

        axes.set_xlabel('Time in simulation (s)')
        axes.set_ylabel('Delay (ms)')
        # axes.set_title("Comparing delay for the three traffic classes", x=0.50)
    elif (args.graph_type == "jitter"):
        values_traffA = [abs(float(v2) - float(v1)) for v1,v2 in zip(values_traffA,values_traffA[1:])]
        values_traffB = [abs(float(v2) - float(v1)) for v1,v2 in zip(values_traffB,values_traffB[1:])]
        values_traffC = [abs(float(v2) - float(v1)) for v1,v2 in zip(values_traffC,values_traffC[1:])]
        axes.set_ylim(bottom=-1, top=max(values_traffA+values_traffB+values_traffC)+10, emit=True, auto=False)
        values.insert(0,0)
        axes.plot(timestamps_qosq_traffA, values_traffA)
        axes.plot(timestamps_qosq_traffB, values_traffB)
        axes.plot(timestamps_qosq_traffC, values_traffC)
        axes.set_xlabel('Time in simulation (s)')
        axes.set_ylabel('Packet jitter compared to previous packet (ms)')
        axes.set_title(args.filename, x=0.50)
    elif (args.graph_type == "metrics"):
        axes.set_ylim(bottom=-0.1, top=max(percent_metric_ok_traffA+percent_metric_ok_traffB+percent_metric_ok_traffC)+0.1, emit=True, auto=False)
        # axes.plot(timestamps_qosq_traffA, [1]*len(timestamps_qosq_traffA), color='black',  )
        # axes.plot(timestamps_qosq_traffA, percent_metric_ok_traffA, color='blue')
        # axes.plot(timestamps_qosq_traffB, percent_metric_ok_traffB, color='green')
        # axes.plot(timestamps_qosq_traffC, percent_metric_ok_traffC, color='red')
        axes.plot(timestamps_qosq_traffA, [1]*len(timestamps_qosq_traffA), color='0.0'  )
        axes.plot(timestamps_qosq_traffA, percent_metric_ok_traffA, color='0.25')
        axes.plot(timestamps_qosq_traffB, percent_metric_ok_traffB, color='0.5')
        axes.plot(timestamps_qosq_traffC, percent_metric_ok_traffC, color='0.75')
        axes.set_xlabel('Time in simulation (s)')
        axes.set_ylabel('Delay (ms)')
        axes.set_title("Comparing metrics for the three traffic classes", x=0.50)
    else :
        print("invalid graph type given (-t [delay|jitter|metrics] )")
        sys.exit(0)
    # Shrink current axis by 20%
    # box = axes[index].get_position()
    # axes[index].set_position([box.x0, box.y0, box.width * 0.8, box.height])

    # Put a legend to the right of the current axis
    # axes.legend(loc='center left', bbox_to_anchor=(1, 0.5)) # throws a warning ?
    if (args.graph_type == "metrics"):
        axes.legend(("IDEAL", "TRAFFIC_A","TRAFFIC_B","TRAFFIC_C"),bbox_to_anchor=(0,1.02,1,0.2), loc="lower left",
                mode="expand", borderaxespad=0, ncol=4)
        # axes.legend(("IDEAL", "TRAFFIC_A","TRAFFIC_B","TRAFFIC_C"),  shadow=True,loc='center left', bbox_to_anchor=(1, 0.5))
    else:
        pass
        # axes.legend(("TRAFFIC_A","TRAFFIC_B","TRAFFIC_C"),bbox_to_anchor=(0,1.02,1,0.2), loc="lower left",
        #         mode="expand", borderaxespad=0, ncol=3)
        # axes.legend(,  shadow=True,loc='center left', bbox_to_anchor=(1, 0.5))
    if(args.output_filename):
        # plt.show()
        plt.savefig(args.output_filename,bbox_inches='tight')
        # savefig()
    else:
        plt.show()
