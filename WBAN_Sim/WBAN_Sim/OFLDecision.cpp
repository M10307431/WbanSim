#include <iostream>
#include <iomanip>
#include <deque>
#include <vector>
#include <fstream>

#include "Struct.h"
#include "OFLDecision.h"

using namespace std;

#define myOFLD 1
#define NOFLD 0
#define AOFLD 2

float Eng = 0;	// (mJ)

float calEnergy(bool remote, int exec, float Eng) {
	
	if(!remote){
		
		Eng =  (p_idle + p_comp)*exec; // + (p_idle + p_trans)*t_trans;
		return Eng;
	}
	else{
		
		Eng =  (p_idle + p_trans + (float)p_comp)*offloadTransfer + (p_idle + p_trans + (float)p_comp)*offloadTransfer;	//+ (p_idle + p_comp)*(exec/speedRatio)
		return Eng;
	}
}

void OFLD(Node* GW){

	if(GW->task_q.empty()){
		printf("GW_%d has't any tasks.\n", GW->id);
	}
	else{
		for(deque<Task>::iterator it=GW->task_q.begin(); it!=GW->task_q.end(); ++it){
			int exec = it->exec;
			
			// calculate energy
			it->localEng = calEnergy(0, exec, Eng);
			it->remoteEng = calEnergy(1, exec, Eng);
			
			// set offloading flag
			if((it->localEng > it->remoteEng)){
				it->offload = true;
				it->target = (exec > 2*offloadTransfer+(exec/speedRatio))? -1 : 999;	// offloading to cloud is slower than origin >> fog
				it->virtualD = (it->target != -1)? (it->deadline-offloadTransfer): (it->deadline-fogTransfer);
			}
			else{
				it->offload = false;
			}

			//////////////////////////////////////
			if(policyOFLD==NOFLD){
				it->offload = false;
			}
			if(policyOFLD==AOFLD){
				it->offload = true;
			}
		}
	}

}

void printOFLD(){

	GW = NodeHead->nextNode;
	fs << "---------- O F L D ------------" << endl;
	while (GW != NULL){
		
		fs << "GW" << GW->id << endl;
		for(deque<Task>::iterator it=GW->task_q.begin(); it!=GW->task_q.end(); ++it){

			fs << "T" << it->id << " " << it->offload << " " << it->localEng << " " << it->remoteEng << "\n";
		}
		cout<<endl;
		GW = GW->nextNode;
	}
}