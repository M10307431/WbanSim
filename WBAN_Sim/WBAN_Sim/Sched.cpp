#include <iostream>
#include <iomanip>
#include <deque>
#include <vector>
#include <fstream>
#include <algorithm>

#include "Struct.h"
#include "Sched.h"
#include "Dispatch.h"
#include "Debug.h"

using namespace std;


/*======== subfunction ============*/
bool minDeadline(Task i, Task j) {return i.deadline < j.deadline;}

void sched_new(Node* GW){
	// task awake or sleep?
	debug(("arrival\r\n"));
	if(!GW->remote_q.wait_q.empty()){
		sort(GW->remote_q.wait_q.begin(), GW->remote_q.wait_q.end(), minDeadline);	// sort by deadline min -> max

		for(deque<Task>::iterator it=GW->remote_q.wait_q.begin(); it!=GW->remote_q.wait_q.end();){
			if(it->deadline <= timeTick){				// task arrival
				it->deadline += it->period;				
				it->remaining = it->exec;
				GW->remote_q.ready_q.push_back(*it);	// push to ready_q
				deque<Task>::iterator nextIt = it+1;	// save next it
				GW->remote_q.wait_q.erase(it,it+1);		// remove from wait_q
				if(!GW->remote_q.wait_q.empty()) {		
					it = nextIt;						// check next task
				}
				else {									// if empty -> break
					break;
				}
			}
			else {
				++it;									// check next task
			}
		}
	}
	if(!GW->local_q.wait_q.empty()){
		sort(GW->local_q.wait_q.begin(), GW->local_q.wait_q.end(), minDeadline);	// sort by deadline min -> max

		for(deque<Task>::iterator it=GW->local_q.wait_q.begin(); it!=GW->local_q.wait_q.end();){
			if(it->deadline <= timeTick){				// task arrival
				it->deadline += it->period;				
				it->remaining = it->exec;
				GW->local_q.ready_q.push_back(*it);		// push to ready_q
				deque<Task>::iterator nextIt = it+1;	// save next it
				GW->local_q.wait_q.erase(it,it+1);		// remove from wait_q
				if(!GW->local_q.wait_q.empty()) {		
					it = nextIt;						// check next task
				}
				else {									// if empty -> break
					break;
				}
			}
			else {
				++it;									// check next task
			}
		}
	}
	debug(("TBS0\r\n"));
	// TBS -> remote_q
	for(deque<Task>::iterator it=GW->TBS.begin(); it!=GW->TBS.end();++it){
		GW->remote_q.ready_q.push_back(*it);
	}
	GW->TBS.clear();
	debug(("TBS1\r\n"));
	// sorting task from minDeadline to maxDeadline
	sort(GW->remote_q.ready_q.begin(), GW->remote_q.ready_q.end(), minDeadline);
	sort(GW->local_q.ready_q.begin(), GW->local_q.ready_q.end(), minDeadline);
	
	// find minDeadline task to execute
	if((!GW->remote_q.ready_q.empty()) && (GW->remote_q.ready_q.begin()->deadline < GW->currTask->deadline)) {   
		if(GW->currTask != idleTask) {	
			GW->remote_q.ready_q.push_back(*GW->currTask);
		}
		GW->currTask = &GW->remote_q.ready_q.front();
		GW->remote_q.ready_q.pop_front();

	}
	else if ((!GW->local_q.ready_q.empty()) && (GW->local_q.ready_q.begin()->deadline < GW->currTask->deadline) && (!GW->currTask->offload)) {
		if(GW->currTask != idleTask) {	
			GW->local_q.ready_q.push_back(*GW->currTask);
		}
		GW->currTask = &GW->local_q.ready_q.front();
		GW->local_q.ready_q.pop_front();
	}
}

void cludServer(Node* GW){
	if(!GW->Cloud.empty()){
		for(deque<Task>::iterator it=GW->Cloud.begin(); it!=GW->Cloud.end();){
			debug(("cloud\r\n"));
			it->remaining-= speedRatio;
			if(it->remaining <= offloadTransfer){
				//if(GW->Cloud.size() >1){
				//	int x =1;
				//}
				it->remaining = offloadTransfer;
				GW->TBS.push_back(*it);
				deque<Task>::iterator prevIt = it-1;	// save next it
				if(it == GW->Cloud.begin()) {
					prevIt = it+1;
				}
				GW->Cloud.erase(it,it+1);
				if(!GW->Cloud.empty()) {		
					it = prevIt+1;						// check next task
				}
				else {									// if empty -> break
					break;
				}
			}
			else {
				++it;
			}
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
		debug(("head\r\n"));
		//while (GW->nextNode != NULL){
			GW = GW->nextNode;
			while(timeTick <= HyperPeriod){
			sched_new(GW);				// schedule new task
			debug(("Sched_new !\r\n"));
			cludServer(GW);				// cloud computing
			debug(("TimeTick : %d\t GW_%d\t T_%d\r\n", timeTick, GW->id, GW->currTask->id));
			printSched();
			if(GW->currTask->offload){
				GW->currTask->remaining--;
				debug(("--_r !\r\n"));
				if(GW->currTask->remaining <= GW->currTask->exec-offloadTransfer && GW->currTask->remaining > offloadTransfer){
					debug(("offload !\r\n"));
					GW->Cloud.push_back(*GW->currTask);
					GW->currTask = idleTask;
					
				}
				else if(GW->currTask->remaining <= 0){
					debug(("finish_r !\r\n"));
					GW->remote_q.wait_q.push_back(*GW->currTask);
					GW->currTask = idleTask;
					
				}
			}
			else{
				GW->currTask->remaining--;
				debug(("--_l !\r\n"));
				if(GW->currTask->remaining <= 0){
					debug(("finish_l !\r\n"));
					GW->local_q.wait_q.push_back(*GW->currTask);
					GW->currTask = idleTask;
					
				}
			}
			timeTick++;
			idleTask->remaining =9999;
		}
		debug(("Next !\r\n"));	
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

void printSched() {

	fs << "TimeTick = " << timeTick << "\tGW_" << GW->id << "\t" << GW->currTask->id << endl;
}