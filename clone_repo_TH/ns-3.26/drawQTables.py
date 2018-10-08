import os,sys
import matplotlib.pyplot as plt
import argparse
import glob

def file_iter(file_ptr):
    while True:
        line = file_ptr.readline()
        if not line:
            break
        yield line

def create_graph(filename):
    data = open(filename, "r")

    timestamps = []
    list_of_values = []
    neighb_dst_combos = []
    final_neighb_dst_combos = []

    changed = False

    for line in file_iter(data):
        if (" -> " in line):
            neighb_dst_combos = line[:-1].split(',')[:-1]
        elif (" >> " in line):
            final_neighb_dst_combos = line[:-1].split(',')[:-1]
        else:
            timestamp,value = line[:-1].split('|')
            timestamps.append(float(timestamp[:-2])/1000000)
            list_value = value.split(',')[:-1]
            for i in xrange(len(list_value)):
                list_value[i] = float(list_value[i][:-2]) / 1000000
            list_of_values.append(list_value)

    if (neighb_dst_combos != final_neighb_dst_combos):
        for i in xrange(len(neighb_dst_combos)):
            if (neighb_dst_combos[i] == final_neighb_dst_combos[i].replace('>>','->')):
                pass
            else:
                print(neighb_dst_combos[i], final_neighb_dst_combos[i])
                #neighbours changed, but not encountered this yet
                raise(Exception("oops"))
        neighb_dst_combos = final_neighb_dst_combos
        changed = True

    for i in xrange(len(neighb_dst_combos)):
        neighb_dst_combos[i] = ("dst " + neighb_dst_combos[i] + " n_h").replace('>>', '->')



    for v in list_of_values:
        try:
            assert(len(v) == len(neighb_dst_combos))
        except Exception as e:
            if changed and len(v) < len(neighb_dst_combos):
                v.extend([0.0] * (len(neighb_dst_combos) - len(v) ))
            elif changed and len(v) >= len(neighb_dst_combos):
                raise(Exception('how'))
            else:
                print(v, neighb_dst_combos)
                sys.exit(0)

    return neighb_dst_combos,timestamps,list_of_values

if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    #Show only those QValues that are related to address values we are interested in
    parser.add_argument('-i', '--interested', type=str, dest="interested", required=False, default=None)
    #Choose which qtable files to show / use...
    parser.add_argument('-t', '--traffic', type=str, dest="traffic", required=True, default=None)
    #Choose which node's qtables to show ?
    parser.add_argument('-n', '--node', type=str, dest="node", required=False, default=None)

    args = parser.parse_args()

    if (len(glob.glob("*_qtable_"+args.traffic+".txt") ) == 0 ) :
        if (len(glob.glob("*_qtable_"+args.traffic+"_graph.txt") ) == 0) :
            print "Oops, no QTable files found. Perhaps re-run the ns3 script?\n"
            sys.exit(1)
        else :
            print "No _qtable_"+args.traffic+".txt files found, using *_qtable_"+args.traffic+"_graph.txt files instead."
    else :
        for f in glob.glob("*_qtable_"+args.traffic+".txt"):
            os.rename(f, f[:-4] + "_graph" + f[-4:] )


    if (not args.node):
        fig, axes = plt.subplots(len(glob.glob("*_qtable_"+args.traffic+"_graph.txt")), sharex=True)
    else:
        fig, axes = plt.subplots()

    index = 0

    for g in glob.glob("*_qtable_"+args.traffic+"_graph.txt"):

        if (args.node and args.node != g.split('_')[0]):
            continue

        labels,timestamps,list_of_values = create_graph(g)

        # # Three subplots sharing both x/y axes
        # f, axes = plt.subplots(len(labels), sharex=True, sharey=True)
        #
        # for i in xrange(len(axes)):
        #     axes[i].plot(timestamps, [item[i] for item in list_of_values])
        #     axes[i].set_title(labels[i])
        # # Fine-tune figure; make subplots close to each other and hide x ticks for
        # # all but bottom plot.
        # f.subplots_adjust(hspace=0)
        # plt.setp([a.get_xticklabels() for a in f.axes[:-1]], visible=True)

        # note that plot returns a list of lines.  The "l1, = plot" usage
        # extracts the first element of the list into l1 using tuple
        # unpacking.  So l1 is a Line2D instance, not a sequence of lines
        for i in xrange(len(labels)):
            if args.interested:
                if any(t in labels[i] for t in args.interested.split(',')):
                    if (not args.node):
                        axes[index].plot(timestamps, [item[i] for item in list_of_values])
                    else:
                        axes.plot(timestamps, [item[i] for item in list_of_values])
            else:
                if (not args.node):
                    axes[index].plot(timestamps, [item[i] for item in list_of_values])
                else:
                    axes.plot(timestamps, [item[i] for item in list_of_values])


        if args.interested:
            labels = [label for label in labels if any(t in label for t in args.interested.split(','))]
        else:
            labels = [label for label in labels ]

        # axes[index].title(figure_title, )
        if (not args.node):
            axes[index].set_xlabel('Time in simulation (ms)')
            axes[index].set_ylabel('Delay (ms)')
            axes[index].set_title("node = " + g.split('_')[0], x=0.50)

            # Shrink current axis by 20%
            box = axes[index].get_position()
            axes[index].set_position([box.x0, box.y0, box.width * 0.8, box.height])

            # Put a legend to the right of the current axis
            # axes[index].legend(loc='center left', bbox_to_anchor=(1, 0.5))
            axes[index].legend(tuple(labels),  shadow=True,loc='center left', bbox_to_anchor=(1, 0.5))

            index += 1
        else:
            axes.set_xlabel('Time in simulation (ms)')
            axes.set_ylabel('Delay (ms)')
            axes.set_title("node = " + g.split('_')[0], x=0.50)

            # Shrink current axis by 20%
            box = axes.get_position()
            axes.set_position([box.x0, box.y0, box.width * 0.93, box.height])

            # Put a legend to the right of the current axis
            # axes[index].legend(loc='center left', bbox_to_anchor=(1, 0.5))
            axes.legend(tuple(labels),  shadow=True,loc='center left', bbox_to_anchor=(1, 0.5))

    for g in glob.glob("*_qtable_"+args.traffic+".txt"):
        os.remove(g)

    plt.show()
