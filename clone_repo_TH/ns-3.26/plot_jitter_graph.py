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
    if (not filename) :
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
        if (len( line[:-1].split(',') ) == 8):
            pktID , timestamp,delay_value , q_value,learning, met_ok , second_to_last_hop , traffictype = line[:-1].split(',')
        else:
            pktID , timestamp,delay_value , q_value,learning , metric_delay , metric_jitter , metric_loss , second_to_last_hop , traffictype = line[:-1].split(',')
        # if (learning == "not_learning"): print(second_to_last_hop)

        if ( metric_jitter == "OK" and metric_delay == "OK" and metric_loss == "OK"):
            metric_ok = "OK"
        else:
            # print(metric_jitter)
            # print(metric_delay)
            # print(metric_loss)
            pass

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
    parser.add_argument('-f_aodv', '--input_aodv', type=str, dest="filename_aodv", required=False, default=None)
    parser.add_argument('-f_q', '--input_q', type=str, dest="filename_q", required=False, default=None)
    parser.add_argument('-f_qosq', '--input_qosq', type=str, dest="filename_qosq", required=False, default=None)

    parser.add_argument('-n', '--dst_node', type=str, dest="dest_node_id", required=False, default=None)

    parser.add_argument('-t', '--type_of_graph', type=str, dest="graph_type", required=False, default="delay")
    parser.add_argument('-i', '--ignore_learning', type=bool, dest="ignore_learning", required=False, default=True)

    parser.add_argument('-of', '--output_filename', type=str, dest="output_filename", required=False, default=None)
    parser.add_argument('-p', '--path', type=str, dest="sim_path", required=False, default="")

    args = parser.parse_args()

    fig, axes = plt.subplots(figsize=(15, 7.5)) #inches for some reason
    index = 0

    if (args.dest_node_id) :
        args.filename_aodv = args.sim_path+"out_stats_aodv_node"+args.dest_node_id+".csv"
        args.filename_q = args.sim_path+"out_stats_q_node"+args.dest_node_id+".csv"
        args.filename_qosq = args.sim_path+"out_stats_qosq_node"+args.dest_node_id+".csv"

    #1st pkt in AODV is RREP fyi
    timestamps_aodv,values_aodv,percent_metric_ok_aodv = create_graph(args.filename_aodv, args.ignore_learning)
    timestamps_q,values_q,percent_metric_ok_q= create_graph(args.filename_q, args.ignore_learning)
    timestamps_qosq,values_qosq,percent_metric_ok_qosq = create_graph(args.filename_qosq, args.ignore_learning)

    axes.set_xlim(left=-10, right=max(timestamps_aodv + timestamps_q + timestamps_qosq)+10, emit=True, auto=False)
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

    if (args.graph_type == "delay"):
        axes.set_ylim(bottom=-1, top=max(values_aodv[1:]+values_q+values_qosq)+10, emit=True, auto=False)
        # axes.plot(timestamps_aodv, values_aodv, color='blue')
        # axes.plot(timestamps_q, values_q, color='green')
        # axes.plot(timestamps_qosq, values_qosq, color='red')
        axes.plot(timestamps_aodv, values_aodv, color='0.0', linewidth = 1.5)
        axes.plot(timestamps_q, values_q, color='0.4', linewidth = 1.5)
        axes.plot(timestamps_qosq, values_qosq, color='0.75', linewidth = 1.5)

        index_q = 2501
        index_aodv = 3780
        index_qosq = 3000

        axes.annotate('AODV',xy=(timestamps_aodv[index_aodv],values_aodv[index_aodv]), xytext=(timestamps_aodv[index_aodv]+450,values_aodv[index_aodv]-50),
            arrowprops=dict(facecolor='white', shrink=0))
        axes.annotate('Q^2',xy=(timestamps_qosq[index_qosq],values_qosq[index_qosq]), xytext=(timestamps_qosq[index_qosq]+1000,values_qosq[index_qosq]+100),
            arrowprops=dict(facecolor='white', shrink=0))
        axes.annotate('EQ',xy=(timestamps_q[index_q],values_q[index_q]), xytext=(timestamps_q[index_q]-600,values_q[index_q]-25),
            arrowprops=dict(facecolor='white', shrink=0))
        # axes.plot(timestamps_aodv, values_aodv, color='0.25', linewidth = 1.5, marker='o')
        # axes.plot(timestamps_q, values_q, color='0.5', linewidth = 1.5, marker='v')
        # axes.plot(timestamps_qosq, values_qosq, color='0.75', linewidth = 1.5, marker='h')
        axes.set_xlabel('Time in simulation (s)')
        axes.set_ylabel('Delay (ms)')
        # axes.set_title("Comparing delay for the three algorithms", x=0.50)
    elif (args.graph_type == "jitter"):
        values_aodv = [abs(float(v2) - float(v1)) for v1,v2 in zip(values_aodv,values_aodv[1:])]
        values_q = [abs(float(v2) - float(v1)) for v1,v2 in zip(values_q,values_q[1:])]
        values_qosq = [abs(float(v2) - float(v1)) for v1,v2 in zip(values_qosq,values_qosq[1:])]
        axes.set_ylim(bottom=-1, top=max(values_aodv+values_q+values_qosq)+10, emit=True, auto=False)
        values.insert(0,0)
        axes.plot(timestamps, values)
        axes.set_xlabel('Time in simulation (s)')
        axes.set_ylabel('Packet jitter compared to previous packet (ms)')
        axes.set_title(args.filename, x=0.50)
    elif (args.graph_type == "metrics"):
        axes.set_ylim(bottom=0.2, top=max(percent_metric_ok_aodv+percent_metric_ok_q+percent_metric_ok_qosq)+0.1, emit=True, auto=False)
        # axes.plot(timestamps_qosq, [1]*len(timestamps_qosq), color='black',  )
        # axes.plot(timestamps_aodv, percent_metric_ok_aodv, color='blue')
        # axes.plot(timestamps_q, percent_metric_ok_q, color='green')
        # axes.plot(timestamps_qosq, percent_metric_ok_qosq, color='red')
        axes.plot(timestamps_qosq, [1]*len(timestamps_qosq), color='0.0', linewidth = 1.5)
        axes.plot(timestamps_aodv, percent_metric_ok_aodv, color='0.25', linewidth = 1.5)#, marker='o)
        axes.plot(timestamps_q, percent_metric_ok_q, color='0.5', linewidth = 1.5)#, marker='v)
        axes.plot(timestamps_qosq, percent_metric_ok_qosq, color='0.75', linewidth = 1.5)#, marker='h)
        print(len(timestamps_q))
        # axes.annotate('EQ',xy=(timestamps_q[4000],percent_metric_ok_q[4000]), xytext=(timestamps_q[4000]-100,percent_metric_ok_q[4000]+0.02),
        #     arrowprops=dict(facecolor='black', shrink=0.5))
        # axes.annotate('Q^2',xy=(timestamps_qosq[4000],percent_metric_ok_qosq[4000]), xytext=(timestamps_qosq[4000]+100,percent_metric_ok_qosq[4000]-0.04),
        #     arrowprops=dict(facecolor='black', shrink=0.5))
        # axes.annotate('AODV',xy=(timestamps_aodv[3500],percent_metric_ok_aodv[3500]), xytext=(timestamps_aodv[3500]-100,percent_metric_ok_aodv[3500]-0.05),
        #     arrowprops=dict(facecolor='black', shrink=0.5))
        # axes.annotate('Ideal',xy=(timestamps_qosq[2000],([1]*len(timestamps_qosq))[2000]), xytext=(timestamps_qosq[2000]+100, ([1]*len(timestamps_qosq))[2000]+0.02),
        #     arrowprops=dict(facecolor='black', shrink=0.5))
        axes.annotate('EQ',xy=(timestamps_q[4000],percent_metric_ok_q[4000]), xytext=(timestamps_q[4000]-100,percent_metric_ok_q[4000]+0.02))
        axes.annotate('Q^2',xy=(timestamps_qosq[4000],percent_metric_ok_qosq[4000]), xytext=(timestamps_qosq[4000]+100,percent_metric_ok_qosq[4000]-0.04))
        axes.annotate('AODV',xy=(timestamps_aodv[7500],percent_metric_ok_aodv[7500]), xytext=(timestamps_aodv[7500]-325,percent_metric_ok_aodv[7500]-0.05))
        axes.annotate('Ideal',xy=(timestamps_qosq[2000],([1]*len(timestamps_qosq))[2000]), xytext=(timestamps_qosq[2000]+100, ([1]*len(timestamps_qosq))[2000]+0.02))
        axes.set_xlabel('Time in simulation (s)')
        axes.set_ylabel('% of packets satisfying traffic constraints')
        # axes.set_title("Comparing delay for the three algorithms", x=0.50)
    else :
        print("invalid graph type given (-t [delay|jitter|metrics] )")
        sys.exit(0)
    # Shrink current axis by 20%
    # box = axes[index].get_position()
    # axes[index].set_position([box.x0, box.y0, box.width * 0.8, box.height])

    # Put a legend to the right of the current axis
    # axes.legend(loc='center left', bbox_to_anchor=(1, 0.5)) # throws a warning ?
    if (args.graph_type == "metrics"):
        # l4 = axes.legend(("Ideal","AODV","EQ","Q^2"),bbox_to_anchor=(0,1.02,1,0.2), loc="lower left",
        # mode="expand", borderaxespad=0, ncol=4)
        pass
        # axes.legend(("Ideal","AODV","EQ","QoS-EQ"),  shadow=True,loc='center left', bbox_to_anchor=(1, 0.5))
    else:
        pass
        axes.legend(("AODV","EQ","Q^2"),bbox_to_anchor=(0,1.02,1,0.2), loc="lower left",
                mode="expand", borderaxespad=0, ncol=3)
        # axes.legend(("AODV","EQ","QoS-EQ"),  shadow=True,loc='center left', bbox_to_anchor=(1, 0.5))
    if(args.output_filename):
        plt.savefig(args.output_filename,bbox_inches='tight')
        plt.show()
    else:
        plt.show()
