#include <iostream>
#include <iomanip>
#include <deque>
#include <vector>
#include <fstream>

#include "Struct_Gen.h"
#include "OFLDecision.h"

using namespace std;

float Eng = 0;	// (mJ)

float calEnergy(bool remote, int exec, float Eng) {
	
	if(!remote){
		
		Eng =  (p_idle + p_comp)*exec; // + (p_idle + p_trans)*t_trans;
		return Eng;
	}
	else{
		
		Eng =  (p_idle + p_trans)*t_trans + (p_idle + p_trans)*t_trans;	//+ (p_idle + p_comp)*(exec/speedRatio)
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
			it->offload = (it->localEng > it->remoteEng)? true : false;
		}
	}

}

void printOFLD(){

	GW = NodeHead->nextNode;
	fs << "---------- O F L D ------------" << endl;
	while (GW != NULL){
		
		fs << "GW " << GW->id << endl;
		for(deque<Task>::iterator it=GW->task_q.begin(); it!=GW->task_q.end(); ++it){

			fs << "T" << it->id << " " << it->offload << " " << it->localEng << " " << it->remoteEng << "\n";
		}
		cout<<endl;
		GW = GW->nextNode;
	}
	cout<<endl;
}