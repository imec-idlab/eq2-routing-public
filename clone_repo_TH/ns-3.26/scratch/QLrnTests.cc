#include "ns3/qlrn-test-base.h"
#include <iostream>
#include <cmath>
#include <fstream>

using namespace ns3;

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  int TEST_GOING_WRONG = -2;
  bool strict_testing = false;
  cmd.AddValue ("t", "set whichever test to focus on (ie skip all others & break after)", TEST_GOING_WRONG);
  cmd.AddValue ("s", "set strict testing on / off : stop the tests upon first failure or no", strict_testing);
  cmd.Parse (argc, argv);

  if (strict_testing ) {
    std::cout << "STRICT TESTING! WILL STOP WHEN A FAILURE IS ENCOUNTERED.\n";
  }

  if (TEST_GOING_WRONG != -2) { std::cout << "Focusing on test case" << TEST_GOING_WRONG << "...:\n"; }

  Time::SetResolution (Time::NS);
  Packet::EnablePrinting();

  // Since std::filesystem isn't usable (it seems), and in the interest of saving time
  // we will simply use this method that just checks files in the directory with increasing
  // index. Painful, but it works
  std::stringstream s, ss;
  std::string path = "QLearningTests/test";
  int i = 0;
  for (; ; i++ ) {
    try {
      s <<  path << i << ".txt";
      ss << path << i << "_expected_results.txt";
      std::ifstream f(s.str());
      std::ifstream ff (ss.str());
      ss.str("");
      s.str("");
      if (!f.good() || !ff.good()) {
        break;
      }
    } catch(std::exception e) {
      std::cout << e.what() << std::endl;
      break;
    }
  }

  int no_passes = 0;
  int no_fails = 0;
  int no_crashes = 0;
  std::vector<int> failed_tests,crashed_tests;

  std::string issue = "";
  std::cout << "Found " << i << " test file(s), executing the tests now (this will take some time):  \n";
  std::cout << "=========================================================================================================================================\n";
  for (int j = 0; j < i; j++) {
    RngSeedManager::SetSeed(1338);
    RngSeedManager::SetRun(j+1);
    QLearningBase obj;

    ss << "test" << j << ".txt";
    s << "test" << j << "_expected_results.txt";


    if (TEST_GOING_WRONG == -2 || j == TEST_GOING_WRONG) {
      std::cout << "\nCommence running test nr " << j+1 << " out of " << i << ": " << s.str() << " --- ";
      std::cout << "Run number = " << RngSeedManager::GetRun() << "  ,   Seed value = " << RngSeedManager::GetSeed() << std::endl;
    } else if (j%25 == 0) {
      std::cout << "Currently at  " << j << std::endl;
    }

    try {
      if (j == TEST_GOING_WRONG) {
        // LogComponentEnable ("QLearnerApplication", LOG_LEVEL_DEBUG);
        // LogComponentEnable ("AodvRoutingProtocol", LOG_LEVEL_DEBUG);
        // LogComponentEnable ("QTable", LOG_LEVEL_DEBUG);
      }
      if (!( obj.ConfigureTest (  j==TEST_GOING_WRONG /* pcap */, false /*printRoutes*/,
                                  false /*linkBreak*/, false /*linkUnBreak*/,
                                  false /*ideal*/, 0 /* numHops */,
                                  ss.str()/* test_case_filename */, s.str()) ) )
      {
        std::stringstream ss;
        ss << "Configuration failed on test"<<j;
        issue = ss.str();
        throw std::exception();
      }
      std::string err = "";

      if (j < TEST_GOING_WRONG) {
        obj.SetTotalTime(36);
      }
      else if (j == TEST_GOING_WRONG+1) {
       break;
      }
      if (TEST_GOING_WRONG == -2 || j == TEST_GOING_WRONG) {
        std::cout << obj.StringTestInfo();
      }
      if (obj.RunTest (err)) {
        if (TEST_GOING_WRONG == -2 || j == TEST_GOING_WRONG) {
          std::cout << "[TEST PASSED]" << err << std::endl;
        }
        no_passes++;
      } else {
        no_fails++;
        failed_tests.push_back(j);
        if (TEST_GOING_WRONG == -2 || j == TEST_GOING_WRONG) {
          std::cout << "[TEST FAILED] --------------------------------- " << err << std::endl;
        }
        if (strict_testing) {
          return 0; // IF STRICT TESTING...
        }
      }
    }
    catch (std::exception e){
      if (TEST_GOING_WRONG == -2 || j == TEST_GOING_WRONG) {
        std::cout << "[TEST CRASHED] --------------------------------- \n" << issue << std::endl;
      }
      crashed_tests.push_back(j);
      no_crashes++;
      if (strict_testing) {
        return 0; // IF STRICT TESTING...
      }
    }
    s.str("");
    ss.str("");
    if (TEST_GOING_WRONG == -2 || j == TEST_GOING_WRONG) {
      std::cout << "=========================================================================================================================================\n";
    }
  }
  std::cout << "Ran " << no_passes+no_fails+no_crashes << " tests. [PASS]: " << no_passes << " /// [FAIL]: " << no_fails << " /// [CRASH]: " << no_crashes << ".\n";
  if (no_fails > 0) {
    std::cout << "Failed tests:   ";
    for (auto i : failed_tests) { std::cout << i << " , " ;}
    std::cout << "\n";
  }
  if (no_crashes > 0) {
    std::cout << "Crashed tests:    ";
    for (auto i : crashed_tests) { std::cout << i << " , " ;}
    std::cout << "\n";
  }

  return 0;
}
