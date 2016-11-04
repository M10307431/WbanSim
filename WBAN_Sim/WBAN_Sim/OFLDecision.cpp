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
#define AOFLDC 2
#define AOFLDF	3

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
			//////////////////////////////////////
			if(policyOFLD==NOFLD){
				it->offload = false;
			}
			else if(policyOFLD==AOFLDC){
				it->offload = true;
			}
			else if(policyOFLD==AOFLDF){
				it->offload = true;
				it->target = (exec > 2*offloadTransfer+(exec/speedRatio))? -1 : 999;	// offloading to cloud is slower than origin >> fog
				it->target = ((it->target == -1)&&(2*fogTransfer+(exec/2) < 2*offloadTransfer+(exec/speedRatio)))? -1 : 999;	// 
				//it->virtualD = (it->target != -1)? (it->deadline-offloadTransfer): (it->deadline-fogTransfer);
			}
			else if(policyOFLD==myOFLD){
				if((it->localEng > it->remoteEng)){
					it->offload = true;
					it->target = (exec > 2*offloadTransfer+(exec/speedRatio))? -1 : 999;	// offloading to cloud is slower than origin >> fog
					it->target = ((it->target == -1)&&(2*fogTransfer+(exec/fogspeed) < 2*offloadTransfer+(exec/speedRatio)))? -1 : 999;	// 
					//it->virtualD = (it->target != -1)? (it->deadline-offloadTransfer): (it->deadline-fogTransfer);
				}
				else{
					it->offload = false;
				}
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