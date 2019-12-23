# eq2-routing
Repository for the ns3 implementation of enhanced Q-routing with QoS https://ieeexplore.ieee.org/abstract/document/8589161

If you use this code, please cite:

T. Hendriks, M. Camelo and S. Latré, "Q2-Routing : A Qos-aware Q-Routing algorithm for Wireless Ad Hoc Networks," 2018 14th International Conference on Wireless and Mobile Computing, Networking and Communications (WiMob), Limassol, 2018, pp. 108-115.
doi: 10.1109/WiMOB.2018.8589161

@INPROCEEDINGS{hendriks2018,
	author={T. {Hendriks} and M. {Camelo} and S. {Latré}},
	booktitle={2018 14th International Conference on Wireless and Mobile Computing, Networking and Communications (WiMob)},
	title={Q2-Routing : A Qos-aware Q-Routing algorithm for Wireless Ad Hoc Networks},
	year={2018},
	volume={},
	number={},
	pages={108-115},
	doi={10.1109/WiMOB.2018.8589161},
	ISSN={2160-4886},
	month={Oct}
}

#How to run the codebase in the clone_repo_TH folder

	1.	./waf configure 
	wait for it to finish succesfully.

	2.	Run any command, the code will build and be installed, then the script will run the simulation as specified via CLI. Note that this is only an example, but this was tested 08/10:
	./waf --run "thomasAODV --doTest=test1.txt --numberOfNodes=3 --traffic=ping --totalTime=300 --eps=0.0 --learn=0.5 --ideal=false"


