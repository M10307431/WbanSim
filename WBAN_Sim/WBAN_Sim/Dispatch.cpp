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
            Priority Setting
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

	// queue assignment
	for(deque<Task>::iterator it=GW->task_q.begin(); it!=GW->task_q.end(); ++it){
		if(it->offload == true){
			it->_setPrio(setPrio(it));
			//it->cnt++;
			it->parent = GW->id;
			it->virtualD = it->deadline;
			if(policyOFLD==myOFLD) {
				it->deadline = (it->target != -1)? (it->period-fogTransfer-(it->exec-2*fogTransfer)) : (it->period-offloadTransfer-(it->exec-2*offloadTransfer)/speedRatio);
			}
			if(policyOFLD==AOFLDC){
					it->deadline--;
			}

			GW->remote_q.ready_q.push_back(*it);
			GW->result.totalTask++; 
		}
		else{
			it->_setPrio(setPrio(it));
			//it->cnt++;
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