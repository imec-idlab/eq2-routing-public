import sys
import argparse
import numpy as np
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
from pylab import savefig

#MUST HAVE : FIRST NODE (0) IS THE ICMP SRC AND LAST NODE IS THE ICMP DST
#Also assuming : 1 interface per node!

# could use https://stackoverflow.com/questions/2301789/read-a-file-in-reverse-order-using-python
# but its small thing. optimizes somewhat, given our assumptions

def file_iter(file_ptr):
    while True:
        line = file_ptr.readline()
        if not line:
            break
        yield line


def parse_aodv_routes(filename):
    data = open(filename, "r")

    #Find the source and destination IP addresses
    ICMP_dst_addr = "0.0.0.0"
    ICMP_src_addr = "0.0.0.0"
    for line in file_iter(data):
        if (len(line[:-1].split("	")) >= 2):
            ICMP_dst_addr = line[:-1].split("	")[2] if line[:-1].split("	")[2] != "127.0.0.1" else ICMP_dst_addr
            if ICMP_src_addr == "0.0.0.0" and line[:-1].split("	")[2] != "":
                ICMP_src_addr = line[:-1].split("	")[2]
    dst_id = int(ICMP_dst_addr.split('.')[-1])-1

    data.seek(0)

    #Create a dict for each node with the next hop in a path to ICMP dst, we use this dict to find out the path
    next_hop_dict = {}
    for line in file_iter(data):
        if line.split(': ')[0] == "Node":
            curr_node = int(line.split(': ')[1].split(';')[0])
        line_split =  line.split("	")
        if len(line_split) > 1:
            if line_split[0] == ICMP_dst_addr:
                next_hop_dict[line_split[2]] = (line_split[1],curr_node)

    data.close()
    path_output = [ICMP_src_addr]
    key = ICMP_src_addr
    while key != ICMP_dst_addr and key != "102.102.102.102":
        if key in next_hop_dict.keys() :
            if (next_hop_dict[key]) in path_output:
                #CYCLE!
                break
            path_output.append(next_hop_dict[key])
            key = next_hop_dict[key][0]
        else:
            key = "102.102.102.102"
    if (path_output[-1][0] == ICMP_dst_addr):
        path_output.append((ICMP_dst_addr,dst_id))
    return path_output, path_output[-1][0] == ICMP_dst_addr

def parse_qlrn_routes(filename):
    data = open(filename, "r")
    path_output = []
    dst_node_id = -1

    for line in file_iter(data):
        if ',' in line:
            nextHop, currNode = line[:-1].split(',')
            path_output.append((nextHop, int(currNode)))
        else:
            if 'srcNode-' in line:
                currNode = line[:-1].split('-')[1]
                path_output.append(currNode)
            elif 'dstNode-' in line:
                dst_addr = line[:-1].split('-')[1]
                dst_node_id = int(dst_addr.split('.')[-1]) - 1
    # print(path_output[-1][0])
    # print(dst_addr)
    # print(path_output)
    if (dst_node_id == -1) :
        print("dont want dst node id not changed")
        sys.exit(111)
    path_output.append((nextHop, dst_node_id))
    return path_output, path_output[-1][0] == dst_addr



def parse_locs(filename):
    data = open(filename, "r")
    x,y,label,color = [],[],[],[]

    color.append("green")
    for line in data.readlines():
        split = line[:-1].split('   ')
        x.append(split[0])
        y.append(split[1])
        label.append(split[3])
        color.append("blue")
    color = color[:-2]
    color.append("red")

    data.close()
    return x,y,color,label

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-l', '--locations', type=str, dest="locations", required=False, default = "locations.txt")
    parser.add_argument('-ar', '--aodvroutes', type=str, dest="aroutes", required=False, default=None)
    parser.add_argument('-qr', '--qlrnroutes', type=str, dest="qroutes", required=False, default=None)
    parser.add_argument('-qqr', '--qosqlrnroutes', type=str, dest="qosqroutes", required=False, default=None)
    parser.add_argument('-of', '--output_filename', type=str, dest="output_filename", required=False, default=None)
    parser.add_argument('-t', '--test_case', type=str, dest="test_case_name", required=False, default=None)
    parser.add_argument('-tt', '--time', type=str, dest="time", required=False, default=None)
    parser.add_argument('-ttt', '--TraffType', type=str, dest="traffType", required=False, default=None)
    parser.add_argument('-p', '--path', type=str, dest="sim_path", required=False, default="")
    args = parser.parse_args()

    if args.sim_path:
        args.locations = args.sim_path + args.locations

    if (args.test_case_name and not args.time) :
        print("Either specify test case & time or specify neither")
        sys.exit(5)
    elif (not args.test_case_name and args.time) :
        print("Either specify test case & time or specify neither")
        sys.exit(6)
    elif (args.test_case_name and args.time) :
        if (args.aroutes or args.qroutes or args.qosqroutes):
            print("dont specify route file if doing it with testcase/time")
            sys.exit(9)
        args.qosqroutes = args.sim_path+ "qosqroute_t"+args.time+"_test_" + args.test_case_name + ".txt"
        # args.qroutes =  args.sim_path+"qroute_t"+args.time+"_test_" + args.test_case_name + ".txt"
        # args.aroutes =  args.sim_path+"aodv_t"+args.time+"_test_" + args.test_case_name + ".txt"

    if (args.traffType):
        args.qosqroutes = args.sim_path+ "qosqroute_t"+args.time+"_test_" + args.test_case_name  + "_" + args.traffType + ".txt"
        args.qroutes =  args.sim_path+"qroute_t"+args.time+"_test_" + args.test_case_name + ".txt"
        args.aroutes =  args.sim_path+"aodv_t"+args.time+"_test_" + args.test_case_name + ".txt"


    fig, ax = plt.subplots(figsize=(11,7.5))

    x,y,colors,labels = parse_locs(args.locations)

    upperlim = max(max(float(xval) for xval in x),max(float(yval) for yval in y)) + 120

    path = []
    q_path = []
    qosq_path = []

    ax.set_xlim(left=-10, right=upperlim, emit=True, auto=False)
    ax.set_ylim(bottom=-130, top=upperlim, emit=True, auto=False)

    if args.aroutes:
        path,path_found = parse_aodv_routes(args.aroutes)
        for node in path[2:-1]:
            colors[node[1]] = "yellow"
        fig.canvas.set_window_title(str(args.aroutes) + "/" + str(args.qroutes))
        if (not path_found):
            print("[AODV] No path was found from the source to the destination so nothing will be done with the aodv.routes input.")
            arrow_aodv = mpatches.Patch(color='grey')
        else:
            if (len(path) == 2):
                arrow_aodv = plt.arrow(float(x[0]), float(y[0]), \
                    float(x[-1]) - float(x[0]), \
                    float(y[-1]) - float(y[0]), \
                      head_width=3, length_includes_head=True, color="grey")
            else:
                for i in xrange(1,len(path)-2):
                    plt.arrow(float(x[int(path[i][1])]), float(y[int(path[i][1])]), \
                        float(x[int(path[i+1][1])]) - float(x[int(path[i][1])]), \
                        float(y[int(path[i+1][1])]) - float(y[int(path[i][1])]), \
                          head_width=3, length_includes_head=True, color="grey")
                i+=1
                arrow_aodv =  plt.arrow(float(x[int(path[i][1])]), float(y[int(path[i][1])]), \
                    float(x[-1]) - float(x[int(path[i][1])]), \
                    float(y[-1]) - float(y[int(path[i][1])]), \
                      head_width=3, length_includes_head=True, color="grey")

    if args.qroutes:
        q_path,path_found = parse_qlrn_routes(args.qroutes)
        if (path == q_path) :
            print("QPath and AODV path were the same for chosen time instances.")
        for node in q_path[2:]:
            colors[node[1]] = "yellow"
        fig.canvas.set_window_title(str(args.aroutes) + "/" + str(args.qroutes))

        if (not path_found):
            print(path)
            print("[QLRN] No path was found from the source to the destination so nothing will be done with the q-routes input.")
            arrow_qlrn = mpatches.Patch(color='darkblue')
        else:
            if (len(q_path) == 2):
                arrow_qlrn = plt.arrow(float(x[0]), float(y[0]), \
                    float(x[-1]) - float(x[0]), \
                    float(y[-1]) - float(y[0]), \
                      head_width=3, length_includes_head=True, color="darkblue")
            else:
                for i in xrange(1,len(q_path)-2):
                    # print("arrow from " + str(q_path[i][1]) + " to " + str(q_path[i+1][1]))
                    plt.arrow(float(x[int(q_path[i][1])]), float(y[int(q_path[i][1])]), \
                        float(x[int(q_path[i+1][1])]) - float(x[int(q_path[i][1])]), \
                        float(y[int(q_path[i+1][1])]) - float(y[int(q_path[i][1])]), \
                          head_width=3, length_includes_head=True, color="darkblue")
                i+=1
                # print(q_path)
                # print("arrow from " + str(q_path[i][1]) + " to " + str(q_path[i+1][1]))
                arrow_qlrn = plt.arrow(float(x[int(q_path[i][1])]), float(y[int(q_path[i][1])]), \
                    float(x[-1]) - float(x[int(q_path[i][1])]), \
                    float(y[-1]) - float(y[int(q_path[i][1])]), \
                      head_width=3, length_includes_head=True, color="darkblue")
    if args.qosqroutes:
        qosq_path,path_found = parse_qlrn_routes(args.qosqroutes)
        if (path == qosq_path) :
            print("QoSQPath and AODV path were the same for chosen time instances.")
        if (q_path == qosq_path):
            print("QoSQPath and qPATH path were the same for chosen time instances.")
        for node in qosq_path[2:]:
            colors[node[1]] = "yellow"
        fig.canvas.set_window_title(str(args.aroutes) + "/" + str(args.qroutes) + "/" + str(args.qosqroutes))

        if (not path_found):
            print("[QLRN] No path was found from the source to the destination so nothing will be done with the q-routes input.")
            arrow_qosqlrn = mpatches.Patch(color='darkblue')
        else:
            if (len(qosq_path) == 2):
                arrow_qosqlrn = plt.arrow(float(x[0]), float(y[0]), \
                    float(x[-1]) - float(x[0]), \
                    float(y[-1]) - float(y[0]), \
                      head_width=3, length_includes_head=True, color="darkblue")
            else:
                for i in xrange(1,len(qosq_path)-1):
                    plt.arrow(float(x[int(qosq_path[i][1])]), float(y[int(qosq_path[i][1])]), \
                        float(x[int(qosq_path[i+1][1])]) - float(x[int(qosq_path[i][1])]), \
                        float(y[int(qosq_path[i+1][1])]) - float(y[int(qosq_path[i][1])]), \
                          head_width=3, length_includes_head=True, color="teal")
                arrow_qosqlrn = plt.arrow(float(x[int(qosq_path[i][1])]), float(y[int(qosq_path[i][1])]), \
                    float(x[-1]) - float(x[int(qosq_path[i][1])]), \
                    float(y[-1]) - float(y[int(qosq_path[i][1])]), \
                      head_width=3, length_includes_head=True, color="teal")

    if (args.qroutes and args.aroutes and args.qosqroutes):
        plt.legend([arrow_aodv, arrow_qlrn, arrow_qosqlrn,], ["AODV","QRoute","QoSQRoute"])
    elif (args.qroutes and args.qosqroutes) :
        plt.legend([ arrow_qlrn, arrow_qosqlrn,], ["QRoute","QoSQRoute"])
    elif (args.aroutes and args.qosqroutes) :
        plt.legend([arrow_aodv, arrow_qosqlrn,], ["AODV","QoSQRoute"])
    elif (args.qroutes) :
        plt.legend([ arrow_qlrn], ["QRoute"])
    elif (args.qosqroutes) :
        plt.legend([ arrow_qosqlrn], ["QoSQRoute"])
    elif (args.aroutes) :
        plt.legend([arrow_aodv], ["AODV"])
    if not args.qroutes and not args.aroutes and not args.qosqroutes:
        fig.canvas.set_window_title(args.locations)

    plt.scatter(x,y, c=colors)
    for i, txt in enumerate(labels):
        ax.annotate(txt, (x[i],y[i]))
    ax.set_xlabel('Metres')
    ax.set_ylabel('Metres')

    if(args.output_filename):
        savefig(args.output_filename)
    plt.show()
