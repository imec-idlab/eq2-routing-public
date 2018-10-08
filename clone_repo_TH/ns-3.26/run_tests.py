import os
import glob
from subprocess import Popen,call,PIPE

if __name__ == "__main__":
    wildcard = '*'
    number_of_tests = len(glob.glob("QLearningTests/test"+wildcard+"_expected_results.txt"))
    number_of_success = 0
    number_of_fails = 0

    print("Found " + str(number_of_tests) + " test file(s), executing the tests now (this may take some time):  ")
    print("==========================================================================================")

    devnull = ('nul' if 'nt' in os.name else '/dev/null')

    for g in glob.glob("QLearningTests/test"+wildcard+"_expected_results.txt"):
        output = call("./waf --run \"thomasAODV --doTest="+g[15:-21]+g[-4:]+" --unit_test_situation \" 1> " + devnull + "", shell=True)
