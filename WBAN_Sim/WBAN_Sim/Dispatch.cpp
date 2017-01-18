#include <iostream>
#include <iomanip>
#include <deque>
#include <vector>
#include <fstream>

#include "Struct.h"
#include "Dispatch.h"
#include "Debug.h"

using namespace std;

#define DEBUG 1
#define NOFLD 0
#define myOFLD 1
#define AOFLDC 2
#define AOFLDF 3
#define SeGW 4

#ifdef DEBUG
#define debug(x) printf x
#endif

void q_init(Node* GW){
	
	GW->local_q.ready_q.clear();
	GW->local_q.wait_q.clear();

	GW->remote_q.ready_q.clear();
	GW->remote_q.wait_q.clear();

	GW->total_U = 0;
}
/****************************************
            Priority Setting (本實驗都是deadline driven，已無使用)
****************************************/
int setPrio(deque<Task>::iterator task){
	switch (task->offload) {
		case true:
			return 0;

		case false:
			return 1;
		
		default:
			return 99;
	}
}

/****************************************
               Dispacher
****************************************/
void dispatch(Node* GW){
	
	q_init(GW);

	/*
		分配task到對應的Q中
		設定parent及時間參數
	*/
	for(deque<Task>::iterator it=GW->task_q.begin(); it!=GW->task_q.end(); ++it){
		if(it->offload == true){
			it->_setPrio(setPrio(it));
			it->parent = GW->id;			// 紀錄parent node (source)
			it->virtualD = it->deadline;	// 紀錄原先的deadline，在sched統一都是看deadline，所以VDeadline是用來記錄原deadline，以便在第二階段回復原deadline使用
			it->uplink = (it->target == -1)? offloadTransfer : fogTransfer;
			it->dwlink = (it->target == -1)? offloadTransfer : fogTransfer;
			it->remaining = it->uplink;
			if(policyOFLD==myOFLD) {
				//it->deadline = (it->target != -1)? (it->period-fogTransfer-it->exec) : (it->period-offloadTransfer-it->exec/speedRatio); // 計算virtual deadline
				it->deadline = (it->target != -1)? (it->period-fogTransfer) : (it->period-offloadTransfer); // 計算virtual deadline
			}
			if(policyOFLD==AOFLDC){
					it->deadline--;
			}

			GW->remote_q.ready_q.push_back(*it);	// 設定好的task丟進Q中
			GW->result.totalTask++; 
		}
		else{
			it->_setPrio(setPrio(it));
			it->parent = GW->id;
			GW->local_q.ready_q.push_back(*it);
			GW->result.totalTask++;
		}
		GW->currTask = idleTask;
	}
}

void printDispatch(){
	
	GW = NodeHead->nextNode;
	fs << "---------- Dispach ------------" << endl;
	while (GW != NULL){
		
		fs << "GW" << GW->id << endl;
		fs << "Local" << endl;

		for(deque<Task>::iterator it=GW->local_q.ready_q.begin(); it!=GW->local_q.ready_q.end(); ++it){

			fs << it->id << " " ;
		}
		fs << endl << "Remote" << endl;
		
		for(deque<Task>::iterator it=GW->remote_q.ready_q.begin(); it!=GW->remote_q.ready_q.end(); ++it){

			fs << it->id << " " ;
		}
		fs << endl;
		GW = GW->nextNode;
	}
	fs << endl;
}