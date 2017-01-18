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
#define SeGW 4

float Eng = 0;	// (mJ)

float calEnergy(bool remote, int exec, float Eng) {		// power model
	
	if(!remote){
		
		Eng =  (p_idle + p_comp)*exec; 
		return Eng;
	}
	else{
		
		Eng =  2*((p_idle + p_trans)*(offloadTransfer-proc) + (p_idle + p_comp )*proc);
		return Eng;
	}
}

void OFLD(Node* GW){

	if(GW->task_q.empty()){
		printf("GW_%d has't any tasks.\n", GW->id);
	}
	else if(policyOFLD == SeGW){	// 如果是SeGW，則採用他自己的決策
		SeGW_OFLD(GW);
	}
	else{
		for(deque<Task>::iterator it=GW->task_q.begin(); it!=GW->task_q.end(); ++it){
			int exec = it->exec;
			
			// calculate energy
			it->localEng = calEnergy(0, exec, Eng);		// 計算local energy
			it->remoteEng = calEnergy(1, exec, Eng);	// 計算remote energy
			
			// set offloading flag
			//////////////////////////////////////
			if(policyOFLD==NOFLD){
				it->offload = false;		// 全都在local執行
			}
			else if(policyOFLD==AOFLDC){
				it->offload = true;			// 全都在cloud執行
				it->vm = NodeNum-1;			// 第三顆node (每顆GW都有一顆VM，在本實驗不考慮cloud partition問題，所以只使用GW3的VM來存放cloud相關資訊)
			}
			else if(policyOFLD==AOFLDF){	// 全都放cloud + fog (已無使用，code也不完整，可忽略) 
				it->offload = true;
				it->target = -1;	// offloading to cloud is slower than origin >> fog
				//it->target = ((it->target == -1)&&(2*fogTransfer+(exec/2) < 2*offloadTransfer+(exec/speedRatio)))? -1 : 999;	// 

			}
			else if(policyOFLD==myOFLD){
				if((it->localEng > it->remoteEng)){		// energy saving
					it->offload = true;
					it->target = (exec > 2*offloadTransfer+(exec/speedRatio))? -1 : 999;	// exec小於cloud時間則放Fog
					//it->target = ((it->target == -1)&&(2*fogTransfer+(exec/fogspeed) > 2*offloadTransfer+(exec/speedRatio)))? -1 : 999;	// 
				}
				else{
					it->offload = false;
				}
			}
		}
	}

}

void SeGW_OFLD(Node* GW){
	if(GW->task_q.empty()){
		printf("GW_%d has't any tasks.\n", GW->id);
	}
	else{
		for(deque<Task>::iterator it=GW->task_q.begin(); it!=GW->task_q.end(); ++it){
			
			// calculate energy
			it->localEng = calEnergy(0, it->exec, Eng);
			it->remoteEng = calEnergy(1, it->exec, Eng);

			// set offloading flag
			if(it->remoteEng > it->localEng){
				it->offload = false;
			}
			else if(it->exec <= 100) {	// 當exec屬於short則放local，在本實驗小於等於最小period則設定為short exec
				it->offload = false;
			}
			else{
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