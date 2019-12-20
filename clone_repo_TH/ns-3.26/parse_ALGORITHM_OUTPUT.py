import argparse

def file_iter(file_ptr):
    while True:
        line = file_ptr.readline()
        if not line:
            break
        yield line[:-1]

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-t', '--test', type=str, dest="test_name", required=True)
    parser.add_argument('-p', '--path', type=str, dest="sim_path", required=False, default=None)
    args = parser.parse_args()

    aodv_learning_per_node = {}
    q_qpackets_per_node = {}
    qosq_qpackets_per_node = {}

    q_lrn_traffic_recv_per_node = {}
    qosq_lrn_traffic_recv_per_node = {}
    q_lrn_traffic_sent_per_node = {}
    qosq_lrn_traffic_sent_per_node = {}


    aodv_total_learning = 0
    q_total_learning = 0
    qosq_total_learning = 0

    aodv_total_data_sent = 0
    q_total_data_sent = 0
    qosq_total_data_sent = 0

    aodv_total_data_recv = 0
    q_total_data_recv = 0
    qosq_total_data_recv = 0

    aodv_avg_delay = 0.0
    q_avg_delay = 0.0
    qosq_avg_delay = 0.0

    if (not args.sim_path):
        args.sim_path = ""


    ### AODV  ###
    aodv_data = open(args.sim_path+"aodvALGORITHM_OUTPUT_test_"+str(args.test_name)+".txt", "r")
    for line in file_iter(aodv_data):
        if ("AODV TRAFFIC" in line):
            index = 0
            for i in line.split("AODV TRAFFIC:   ")[1].split('/')[:-1]:
                aodv_total_learning += int(i)
                aodv_learning_per_node["node"+str(index)] = int(i)
                index += 1
        elif ("DATA TRAFFIC" in line):
            for i in line.split("DATA TRAFFIC:   ")[1].split('/')[:-1]:
                aodv_total_data_sent += int(i)
        elif ("DATA REC TRAFFIC" in line):
            aodv_total_data_recv += int(line.split("DATA REC TRAFFIC:   ")[1].split('/')[-2])
        elif ("Observed average delay" in line):
            aodv_avg_delay = float(line.split('+')[1].split('ms')[0])

    ### QOSQ  ###
    qosq_data = open(args.sim_path+"qosqALGORITHM_OUTPUT_test_"+str(args.test_name)+".txt", "r")
    for line in file_iter(qosq_data):
        if ("QLRN TRAFFIC" in line):
            index = 0
            for i in line.split("QLRN TRAFFIC:   ")[1].split('/')[:-1]:
                qosq_total_learning += int(i)
                qosq_qpackets_per_node["node"+str(index)] = int(i)
                index += 1
        elif ("LRN REC TRAFFIC" in line):
            index = 0
            for i in line.split("LRN REC TRAFFIC:   ")[1].split('/')[:-1]:
                qosq_lrn_traffic_sent_per_node["node"+str(index)] = int(i)
                index += 1
        elif ("LRN TRAFFIC" in line):
            index = 0
            for i in line.split("LRN TRAFFIC:   ")[1].split('/')[:-1]:
                qosq_total_learning += int(i)
                qosq_lrn_traffic_recv_per_node["node"+str(index)] = int(i)
                index += 1
        elif ("DATA TRAFFIC" in line):
            for i in line.split("DATA TRAFFIC:   ")[1].split('/')[:-1]:
                qosq_total_data_sent += int(i)
        elif ("DATA REC TRAFFIC" in line):
            qosq_total_data_recv += int(line.split("DATA REC TRAFFIC:   ")[1].split('/')[-2])
        elif ("Observed average delay" in line):
            qosq_avg_delay = float(line.split('+')[1].split('ms')[0])

    ### Q     ###
    q_data = open(args.sim_path+"qALGORITHM_OUTPUT_test_"+str(args.test_name)+".txt", "r")
    for line in file_iter(q_data):
        if ("QLRN TRAFFIC" in line):
            index = 0
            for i in line.split("QLRN TRAFFIC:   ")[1].split('/')[:-1]:
                q_total_learning += int(i)
                q_qpackets_per_node["node"+str(index)] = int(i)
                index += 1
        elif ("LRN REC TRAFFIC" in line):
            index = 0
            for i in line.split("LRN REC TRAFFIC:   ")[1].split('/')[:-1]:
                q_lrn_traffic_sent_per_node["node"+str(index)] = int(i)
                index += 1
        elif ("LRN TRAFFIC" in line):
            index = 0
            for i in line.split("LRN TRAFFIC:   ")[1].split('/')[:-1]:
                q_total_learning += int(i)
                q_lrn_traffic_recv_per_node["node"+str(index)] = int(i)
                index += 1
        elif ("DATA TRAFFIC" in line):
            for i in line.split("DATA TRAFFIC:   ")[1].split('/')[:-1]:
                q_total_data_sent += int(i)
        elif ("DATA REC TRAFFIC" in line):
            q_total_data_recv += int(line.split("DATA REC TRAFFIC:   ")[1].split('/')[-2])
        elif ("Observed average delay" in line):
            q_avg_delay = float(line.split('+')[1].split('ms')[0])
    to_key = lambda key: "node"+str(key)

    print("\n\n\n#########################\n### AODV ################\n#########################\n")
    print("Total data packets sent in the available time : " + str(aodv_total_data_sent) + " of which " + str(aodv_total_data_recv) +
        " arrived at dst, or " + str(float(aodv_total_data_recv) / float(aodv_total_data_sent)*100) + "%."  )
    print("Avg delay in AODV traffic was " + str(aodv_avg_delay))
    print("Total number of packets sent for learning : " + str(aodv_total_learning) + ". Per node, this comes out to (mostly hello pkts, some RREQ/RREP)")
    i = 0
    last_row = len(aodv_learning_per_node.keys())%3
    while i < len(aodv_learning_per_node)-2 :
        print(to_key(i).ljust(10) + ":     " + str(aodv_learning_per_node[to_key(i)]).ljust(17) + "    ||    "
                + to_key(i+1).ljust(10) + ":     " + str(aodv_learning_per_node[to_key(i+1)]).ljust(17)+ "    ||    "
                + to_key(i+2).ljust(10) + ":     " + str(aodv_learning_per_node[to_key(i+2)]).ljust(17))
        i += 3
    if (last_row != 0):
        print(to_key(i).ljust(10) + ":     " + str(aodv_learning_per_node[to_key(i)]).ljust(17) + "    ||    "
                + ( to_key(i+1).ljust(10) + ":     " + str(aodv_learning_per_node[to_key(i+1)]).ljust(17)+ "    ||    " if last_row == 2 else ""))

    print("\n\n\n#########################\n### QOSQ ################\n#########################\n")
    print("Total data packets sent in the available time : " + str(qosq_total_data_sent) + " of which " + str(qosq_total_data_recv) +
        " arrived at dst, or " + str(float(qosq_total_data_recv) / float(qosq_total_data_sent)*100) + "%."  )
    print("Avg delay in QOSQ traffic was " + str(qosq_avg_delay))
    print("Total number of packets used for learning (as sent lrn traffic and the QLrnHeader responses): " + str(qosq_total_learning) + ". Per node, this is QPACKETS,LEARNING TRAFFIC SENT,LEARNING TRAFFIC RECEIVED" )
    i = 0
    last_row = len(qosq_qpackets_per_node.keys())%3
    while i < len(qosq_qpackets_per_node)-2 :
        print(to_key(i).ljust(10) + ":     " + str(qosq_qpackets_per_node[to_key(i)]).ljust(5) + ","
                    + str(qosq_lrn_traffic_recv_per_node[to_key(i)]).ljust(5)   + "," + str(qosq_lrn_traffic_sent_per_node[to_key(i)]).ljust(5)  + "    ||    "
                + to_key(i+1).ljust(10) + ":     " + str(qosq_qpackets_per_node[to_key(i+1)]).ljust(5) + ","
                    + str(qosq_lrn_traffic_recv_per_node[to_key(i+1)]).ljust(5) + "," + str(qosq_lrn_traffic_sent_per_node[to_key(i+1)]).ljust(5) + "    ||    "
                + to_key(i+2).ljust(10) + ":     " + str(qosq_qpackets_per_node[to_key(i+2)]).ljust(5) + ","
                    + str(qosq_lrn_traffic_recv_per_node[to_key(i+2)]).ljust(5) + "," + str(qosq_lrn_traffic_sent_per_node[to_key(i+2)]).ljust(5) )
        i += 3
    if (last_row != 0):
        print(to_key(i).ljust(10) + ":     " + str(qosq_qpackets_per_node[to_key(i)]).ljust(5) + "," + str(qosq_lrn_traffic_recv_per_node[to_key(i)]).ljust(5)  + ","
                + str(qosq_lrn_traffic_sent_per_node[to_key(i)]).ljust(5)+ "    ||    "
            + ( to_key(i+1).ljust(10) + ":     " + str(qosq_qpackets_per_node[to_key(i+1)]).ljust(5) + "," + str(qosq_lrn_traffic_recv_per_node[to_key(i+1)]).ljust(5) + ","
                + str(qosq_lrn_traffic_sent_per_node[to_key(i+1)]).ljust(5)+ "    ||    " if last_row == 2 else ""))


    print("\n\n\n#########################\n### Q    ################\n#########################\n")
    print("Total data packets sent in the available time : " + str(q_total_data_sent) + " of which " + str(q_total_data_recv) +
        " arrived at dst, or " + str(float(q_total_data_recv) / float(q_total_data_sent)*100) + "%."  )
    print("Avg delay in Q    traffic was " + str(q_avg_delay))
    print("Total number of packets used for learning (as sent lrn traffic and the QLrnHeader responses): " + str(q_total_learning) + ". Per node, this is QPACKETS,LEARNING TRAFFIC SENT,LEARNING TRAFFIC RECEIVED" )
    i = 0
    last_row = len(q_qpackets_per_node.keys())%3
    while i < len(q_qpackets_per_node)-2 :
        print(to_key(i).ljust(10) + ":     " + str(qosq_qpackets_per_node[to_key(i)]).ljust(5) + ","
                    + str(q_lrn_traffic_recv_per_node[to_key(i)]).ljust(5)   + "," + str(q_lrn_traffic_sent_per_node[to_key(i)]).ljust(5)  + "    ||    "
                + to_key(i+1).ljust(10) + ":     " + str(qosq_qpackets_per_node[to_key(i+1)]).ljust(5) + ","
                    + str(q_lrn_traffic_recv_per_node[to_key(i+1)]).ljust(5) + "," + str(q_lrn_traffic_sent_per_node[to_key(i+1)]).ljust(5) + "    ||    "
                + to_key(i+2).ljust(10) + ":     " + str(qosq_qpackets_per_node[to_key(i+2)]).ljust(5) + ","
                    + str(q_lrn_traffic_recv_per_node[to_key(i+2)]).ljust(5) + "," + str(q_lrn_traffic_sent_per_node[to_key(i+2)]).ljust(5) )
        i += 3
    if (last_row != 0):
        print(to_key(i).ljust(10) + ":     " + str(q_qpackets_per_node[to_key(i)]).ljust(5) + "," + str(q_lrn_traffic_recv_per_node[to_key(i)]).ljust(5)  + ","
                + str(q_lrn_traffic_sent_per_node[to_key(i)]).ljust(5)+ "    ||    "
            + ( to_key(i+1).ljust(10) + ":     " + str(q_qpackets_per_node[to_key(i+1)]).ljust(5) + "," + str(q_lrn_traffic_recv_per_node[to_key(i+1)]).ljust(5) + ","
                + str(q_lrn_traffic_sent_per_node[to_key(i+1)]).ljust(5)+ "    ||    " if last_row == 2 else ""))


# node0(src)/node1/node2/node3/node4/node5/node6/node7/node8/node9/node10/node11/node12/node13/node14/node15/node16/node17/node18(dst)/
# 772(772)/522(522)/355(355)/377(377)/483(483)/15(15)/29(29)/479(479)/35(35)/47(47)/24(24)/18(18)/6(6)/5(5)/2(2)/2(2)/13(13)/11(11)/470(470)/
# QLRN TRAFFIC:   772/522/355/377/483/15/29/479/35/47/24/18/6/5/2/2/13/11/470/
# DATA TRAFFIC:   11921/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/
# LRN TRAFFIC:    585/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/
# DATA REC TRAFFIC:   574/12121/211/179/11945/0/0/11929/0/0/0/0/0/0/0/0/241/28/11921/
# LRN REC TRAFFIC:    641/299/284/316/252/15/29/244/35/46/24/17/5/3/1/1/12/10/236/
# Observed average delay at 10.1.1.19 was +40.89341199999999999998ms.
#
# ################################################################################################################################################################################
# ################################################################################################################################################################################
# ################################################################################################################################################################################
# ################################################################################################################################################################################
#
# node0(src)/node1/node2/node3/node4/node5/node6/node7/node8/node9/node10/node11/node12/node13/node14/node15/node16/node17/node18(dst)/
# 790(790)/424(424)/360(360)/379(379)/365(365)/5(5)/3(3)/367(367)/11(11)/9(9)/7(7)/9(9)/4(4)/6(6)/2(2)/3(3)/24(24)/18(18)/359(359)/
# QLRN TRAFFIC:   790/424/360/379/365/5/3/367/11/9/7/9/4/6/2/3/24/18/359/
# AODV TRAFFIC:   5/3/4/4/4/2/2/2/4/4/3/3/3/3/4/4/2/2/1/
# DATA TRAFFIC:   11921/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/
# LRN TRAFFIC:    571/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/
# DATA REC TRAFFIC:   301/11926/163/134/11922/0/0/11933/0/0/0/0/0/0/0/0/25/10/11921/
# LRN REC TRAFFIC:    690/322/305/333/263/5/3/265/10/8/6/8/2/4/1/2/19/15/252/
# Observed average delay at 10.1.1.19 was +40.53085599999999999996ms.
# Observed packet delivery ratio from source 10.1.1.1 toward destination 10.1.1.19 going via 10.1.1.2 was 1.
# Observed packet delivery ratio from source 10.1.1.1 toward destination 10.1.1.19 going via 10.1.1.3 was 1.
# Observed packet delivery ratio from source 10.1.1.1 toward destination 10.1.1.19 going via 10.1.1.4 was 1.
#     Observed (final) QRoute from 10.1.1.1 = 0-1-4-7-18-
#     Observed (final) QRoute from 10.1.1.2 = 1-4-7-18-
#     Observed (final) QRoute from 10.1.1.3 = 2-0-1-4-7-18-
