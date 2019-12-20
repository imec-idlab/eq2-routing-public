# eq2-routing
Repository for the ns3 implementation of enhanced Q-routing with QoS

#How to run the codebase in the clone_repo_TH folder

	1.	./waf configure 
	wait for it to finish succesfully.

	2.	Run any command, the code will build and be installed, then the script will run the simulation as specified via CLI. Note that this is only an example, but this was tested 08/10:
	./waf --run "thomasAODV --doTest=test1.txt --numberOfNodes=3 --traffic=ping --totalTime=300 --eps=0.0 --learn=0.5 --ideal=false"


