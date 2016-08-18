#include <iostream>
#include <iomanip>
#include <deque>
#include <vector>
#include <fstream>
#include <algorithm>

#include "Struct.h"
#include "Sched.h"

using namespace std;

/*======== subfunction ============*/
bool minDeadline(Task i, Task j) {return i.deadline < j.deadline;}

void sched_new(Node* GW){
	// task awake or sleep?
	sort(GW->remote_q.wait_q.begin(), GW->remote_q.wait_q.end(), minDeadline);
	sort(GW->local_q.wait_q.begin(), GW->local_q.wait_q.end(), minDeadline);
	if(!GW->remote_q.wait_q.empty()){
		while(GW->remote_q.wait_q.begin()->deadline <= timeTick){
			GW->remote_q.wait_q.begin()->deadline += GW->remote_q.wait_q.begin()->period;
			GW->remote_q.wait_q.begin()->remaining = GW->remote_q.wait_q.begin()->exec;
			GW->remote_q.ready_q.push_back(GW->remote_q.wait_q.front());	// remote task arrival -> put in ready queue
			GW->remote_q.wait_q.pop_front();
		}
	}
	if(!GW->local_q.wait_q.empty()){
		while(GW->local_q.wait_q.begin()->deadline <= timeTick){
			GW->local_q.wait_q.begin()->deadline += GW->local_q.wait_q.begin()->period;
			GW->local_q.wait_q.begin()->remaining = GW->local_q.wait_q.begin()->exec;
			GW->local_q.ready_q.push_back(GW->local_q.wait_q.front());	// local task arrival -> put in ready queue
			GW->local_q.wait_q.pop_front();
		}
	}
	// TBS -> remote_q
	for(deque<Task>::iterator it=GW->TBS.begin(); it!=GW->TBS.end();){
		GW->remote_q.ready_q.push_back(*it);
	}
	GW->TBS.clear();

	// sorting task from minDeadline to maxDeadline
	sort(GW->remote_q.ready_q.begin(), GW->remote_q.ready_q.end(), minDeadline);
	sort(GW->local_q.ready_q.begin(), GW->local_q.ready_q.end(), minDeadline);
	
	// find minDeadline task to execute
	if(!GW->remote_q.wait_q.empty()){
		if((GW->remote_q.ready_q.begin()->deadline <= GW->local_q.ready_q.begin()->deadline) && (GW->remote_q.ready_q.begin()->deadline < GW->currTask->deadline)){   // q empty
			GW->currTask = &GW->remote_q.ready_q.front();
			GW->remote_q.ready_q.pop_front();
		}
	else if (GW->local_q.ready_q.begin()->deadline < GW->currTask->deadline) {
		GW->currTask = &GW->local_q.ready_q.front();
		GW->local_q.ready_q.pop_front();
	}
}

void cludServer(Node* GW){
	if(!GW->Cloud.empty()){
		for(deque<Task>::iterator it=GW->Cloud.begin(); it!=GW->Cloud.end();){
			it->remaining-= speedRatio;
			if(it->remaining <= offloadTransfer){
				it->remaining = offloadTransfer;
				GW->TBS.push_back(*it);
				GW->Cloud.erase(it,it+1);
			}
			else
				++it;
		}
	}
}

/*=====================
	    Policy
=====================*/
void EDF(){
	timeTick = 0;

	while(timeTick <= HyperPeriod){
		GW = NodeHead;
		while (GW->nextNode != NULL){
			GW = GW->nextNode;
			sched_new(GW);				// schedule new task
			cludServer(GW);				// cloud computing
			if(GW->currTask->offload){
				GW->currTask->remaining--;
				if(GW->currTask->remaining <= GW->currTask->exec-offloadTransfer){
					GW->Cloud.push_back(*GW->currTask);
					GW->currTask = idleTask;
				}
				else if(GW->currTask->remaining <= 0){
					GW->remote_q.wait_q.push_back(*GW->currTask);
					GW->currTask = idleTask;
				}
			}
			else{
				GW->currTask->remaining--;
				if(GW->currTask->remaining <= 0){
					GW->local_q.wait_q.push_back(*GW->currTask);
					GW->currTask = idleTask;
				}
			}

		}
	
		timeTick++;
		idleTask->remaining =9999;
	}

}

void scheduler(int policy){
	switch (policy)
	{
	case 0:
		break;

	case 1:
		break;

	case 2:
		EDF();
		break;
	
	default:
		EDF();
		break;
	}
}