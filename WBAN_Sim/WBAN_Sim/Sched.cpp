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

#define myOFLD 1
#define NOFLD 0
#define AOFLDC 2
#define AOFLDF	3
#define SeGW 4

/*======== subfunction ============*/
bool minDeadline(Task i, Task j) {return (i.deadline < j.deadline || (i.deadline == j.deadline && i.remaining < j.remaining));}

void sched_new(Node* GW){
	// task awake or sleep?
	debug(("arrival\r\n"));
	if(!GW->remote_q.wait_q.empty()){
		sort(GW->remote_q.wait_q.begin(), GW->remote_q.wait_q.end(), minDeadline);	// sort by deadline min -> max

		for(deque<Task>::iterator it=GW->remote_q.wait_q.begin(); it!=GW->remote_q.wait_q.end();){
			if(it->deadline <= timeTick){				// task arrival
				it->deadline += it->period;
				it->virtualD = it->deadline;
				it->remaining = it->exec;
				it->cnt += 1;

				if(it->deadline <= HyperPeriod){
					GW->result.totalTask++;
				}
				if(policyOFLD==myOFLD) {
					it->deadline = (it->target != -1)? (it->virtualD-fogTransfer-(it->exec-2*fogTransfer)) : (it->virtualD-offloadTransfer-(it->exec-2*offloadTransfer)/speedRatio);	//virtual deadline
				}

				if(policyOFLD==AOFLDC){
					it->deadline--;
				}

				GW->remote_q.ready_q.push_front(*GW->currTask);		// currTask 暫存，不知明原因執行push時會將currTask修改到
				GW->remote_q.ready_q.push_back(*it);					// push to ready_q
				GW->currTask = &GW->remote_q.ready_q.front();
				GW->remote_q.ready_q.pop_front();

				//deque<Task>::iterator nextIt = it+1;	// save next it
				it = GW->remote_q.wait_q.erase(it,it+1);		// remove from wait_q
				//if(!GW->remote_q.wait_q.empty()) {		
				//	it = nextIt;						// check next task
				//}
				//else {									// if empty -> break
				//	break;
				//}
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
				it->cnt += 1;

				if(it->deadline <= HyperPeriod){
					GW->result.totalTask++;
				}

				GW->local_q.ready_q.push_front(*GW->currTask);		// currTask 暫存，不知明原因執行push時會將currTask修改到
				GW->local_q.ready_q.push_back(*it);					// push to ready_q
				GW->currTask = &GW->local_q.ready_q.front();
				GW->local_q.ready_q.pop_front();

				//deque<Task>::iterator nextIt = it+1;	// save next it
				it = GW->local_q.wait_q.erase(it,it+1);		// remove from wait_q
				//if(!GW->local_q.wait_q.empty()) {		
				//	it = nextIt;						// check next task
				//}
				//else {									// if empty -> break
				//	break;
				//}
			}
			else {
				++it;									// check next task
			}
		}
	}

	// sorting task from minDeadline to maxDeadline
	sort(GW->remote_q.ready_q.begin(), GW->remote_q.ready_q.end(), minDeadline);
	sort(GW->local_q.ready_q.begin(), GW->local_q.ready_q.end(), minDeadline);
	
	// find minDeadline task to execute
	if((!GW->remote_q.ready_q.empty()) && (GW->remote_q.ready_q.begin()->deadline < GW->currTask->deadline)) {   
		if(GW->currTask->id != idleTask->id){
			if(GW->currTask->offload){
				GW->remote_q.ready_q.push_back(*GW->currTask);
			}
			else {
				GW->local_q.ready_q.push_back(*GW->currTask);
			}
		}

		GW->currTask = &GW->remote_q.ready_q.front(); Time("context switch"); // get the min deadline task
		GW->remote_q.ready_q.pop_front();
		
		// not finish task
		if((GW->currTask->deadline <= timeTick)&&(GW->currTask->remaining > 0)){
			debug(("miss_rq !\r\n")); Time("miss_rq");
			// fog migration task miss -> parent GW waiting_q
			if(GW->currTask->parent != GW->id){
				Node *parent = new Node;
				parent = backMigraSrc(GW->currTask);
				parent->result.miss++;
				GW->currTask->deadline = GW->currTask->virtualD;	// restore origin deadline
				parent->remote_q.wait_q.push_back(*GW->currTask);
				parent =NULL;
				delete parent;
			}
			else{
				GW->result.miss++;
				GW->remote_q.wait_q.push_back(*GW->currTask);
			}
			GW->currTask = idleTask;
			sched_new(GW);
		}

	}

	if ((!GW->local_q.ready_q.empty()) && (GW->local_q.ready_q.begin()->deadline < GW->currTask->deadline)) {
		if(GW->currTask->id != 999) {	
			if(GW->currTask->offload){
				GW->remote_q.ready_q.push_back(*GW->currTask);
			}
			else {
				GW->local_q.ready_q.push_back(*GW->currTask);
			}
		}
		GW->currTask = &GW->local_q.ready_q.front(); Time("context switch");
		GW->local_q.ready_q.pop_front();
		if((GW->currTask->deadline <= timeTick)&&(GW->currTask->remaining > 0)){
			debug(("miss_lq !\r\n")); Time("miss_lq");
			GW->result.miss++;
			GW->local_q.wait_q.push_back(*GW->currTask);
			GW->currTask = idleTask;
			sched_new(GW);
		}	
	}
}

void sched_fifo(Node* GW){
	// check miss task in ready_q
	if(!GW->remote_q.ready_q.empty()) {   
		for(deque<Task>::iterator it=GW->remote_q.ready_q.begin(); it!=GW->remote_q.ready_q.end();){
			if(it->deadline <= timeTick && it->remaining > 0){				// task arrival
				GW->currTask = &GW->remote_q.ready_q.at(it-GW->remote_q.ready_q.begin()); // 為了Time func的顯示，暫時用currTask來承接it所指的任務，顯示完畢還回idle
				debug(("miss_rq !\r\n")); Time("miss_rq");
				GW->currTask = idleTask;
				GW->result.miss++;
				GW->remote_q.wait_q.push_back(*it);

				it = GW->remote_q.ready_q.erase(it);	// remove from wait_q
				/*if(!GW->remote_q.ready_q.empty()) {		
					break;
				}*/
			}
			else {
				++it;									// check next task
			}
		}
	}
	if(!GW->local_q.ready_q.empty()) {   
		for(deque<Task>::iterator it=GW->local_q.ready_q.begin(); it!=GW->local_q.ready_q.end();){
			if(it->deadline <= timeTick && it->remaining > 0){				// task arrival
				GW->currTask = &GW->local_q.ready_q.at(it-GW->local_q.ready_q.begin()); // 為了Time func的顯示，暫時用currTask來承接it所指的任務，顯示完畢還回idle
				debug(("miss_rq !\r\n")); Time("miss_rq");
				GW->currTask = idleTask;
				GW->result.miss++;
				GW->local_q.wait_q.push_back(*it);

				it = GW->local_q.ready_q.erase(it);	// remove from wait_q
				/*if(!GW->local_q.ready_q.empty()) {		
					break;
				}*/
			}
			else {
				++it;									// check next task
			}
		}
	}
	// task awake or sleep?
	debug(("arrival\r\n"));
	if(!GW->remote_q.wait_q.empty()){

		sort(GW->remote_q.wait_q.begin(), GW->remote_q.wait_q.end(), minDeadline);	// sort by deadline min -> max

		for(deque<Task>::iterator it=GW->remote_q.wait_q.begin(); it!=GW->remote_q.wait_q.end();){
			if(it->deadline <= timeTick){				// task arrival
				it->deadline += it->period;
				it->remaining = it->exec;
				it->cnt += 1;

				if(it->deadline <= HyperPeriod){
					GW->result.totalTask++;
				}

				GW->remote_q.ready_q.push_back(*it);					// push to ready_q

				it = GW->remote_q.wait_q.erase(it);		// remove from wait_q
				/*if(!GW->remote_q.wait_q.empty()) {		
					break;
				}*/
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
				it->cnt += 1;

				if(it->deadline <= HyperPeriod){
					GW->result.totalTask++;
				}

				GW->local_q.ready_q.push_back(*it);					// push to ready_q

				it = GW->local_q.wait_q.erase(it);		// remove from wait_q
				//if(GW->local_q.wait_q.empty()) {		
				//	break;
				//}
			}
			else {
				++it;									// check next task
			}
		}
	}
	// sched_new  prio: remote > local
	
	if(!GW->local_q.ready_q.empty()){
		GW->currTask = &GW->local_q.ready_q.front(); Time("context switch");
		GW->local_q.ready_q.pop_front();
	}
	else if(!GW->remote_q.ready_q.empty()){
		GW->currTask = &GW->remote_q.ready_q.front(); Time("context switch");
		GW->remote_q.ready_q.pop_front();
	}
	else{
		GW->currTask = idleTask;
	}

}


int traffic = 0;

void cludServer(Node* GW){
	if(!GW->Cloud.empty()){
		for(deque<Task>::iterator it=GW->Cloud.begin(); it!=GW->Cloud.end();){
			debug(("cloud\r\n"));

			if(it->deadline <= timeTick && it->remaining > offloadTransfer){
				
				GW->remote_q.wait_q.push_back(*it);
				GW->result.miss++;

				it = GW->Cloud.erase(it,it+1);
			}
			else{
				if(speedRatio-traffic>=1){
					it->remaining-= (speedRatio-traffic);
				}
				else{
					it->remaining-= 1;
				}
				if(it->remaining <= 0){
					it->remaining = offloadTransfer;
					GW->TBS.push_back(*it);
					it = GW->Cloud.erase(it,it+1);
				}
				else {
					++it;
				}
			}
		}
	}
}

/*=====================
	    Migration
=====================*/
Node *target = new Node;
Node *parent = new Node;
int battSum = 0;
float utiSum = 0.0;

//============================== Evaluate migration target ============================
void EvaluationFog(Task *task){
	target = NodeHead;

	while(target->nextNode != NULL){
		target = target->nextNode;
		target->MW(battSum, utiSum, task->deadline);
		//printf("(%d,%f,%d,%f)\t",target->batt,target->current_U,target->block,target->migratWeight);
	}
	
	target = NodeHead;
	float temp_MW = target->nextNode->migratWeight;
	while (target->nextNode != NULL) {
		target = target->nextNode;
		if((target->id != task->parent)&&(target->migratWeight > temp_MW)&&(1.0-target->current_U > task->uti)){
			task->target = target->id;
			/*task->virtualD = (task->remaining-2*fogTransfer)/(1.0-target->current_U);
			if(task->virtualD > task->deadline-fogTransfer ) {
				task->virtualD = task->deadline-fogTransfer;
			}*/
		}
	}
	
	// can not find proper dest >> local running
	if(task->target == 999 || task->target == task->parent){
		task->target = task->parent;
		task->deadline = task->virtualD;	// original deadline
	}
	target = NULL;
}
//============================== Retuen migration target ============================
Node *findMigraDest(Task *task){
	
	target = NodeHead;
	while (target->nextNode != NULL) {
		target = target->nextNode;
		if(target->id == task->target){
			return target;
		}
	}
	Time(("Migration Error!!\r\n"));
	target = NULL;
	return target;
}

Node *backMigraSrc(Task *task){
	parent = NodeHead;
	while (parent->nextNode != NULL) {
		parent = parent->nextNode;
		if(parent->id == task->parent){
			return parent;
		}
	}
	Time(("Migration back Error!!\r\n"));
	parent = NULL;
	return parent;
}

void Migration(Node* src, Node* dest){
	
	dest->TBS.push_back(*src->currTask);
	src->currTask = idleTask;
}



/*=====================
	    Policy
=====================*/
void EDF(){
	timeTick = 0;
	
	while(timeTick <= HyperPeriod+1){
		/*if(timeTick == HyperPeriod){
			int a =1;
		}*/
		GW = NodeHead;
		battSum = 0;
		utiSum = 0.0;
		traffic = 0;
		debug(("head\r\n"));

		while (GW->nextNode != NULL){
			GW = GW->nextNode;
			debug(("TBS0\r\n"));
			// TBS -> remote_q
			while(!GW->TBS.empty()){
				
				GW->remote_q.ready_q.push_front(*GW->currTask);		// currTask 暫存，不知明原因執行push時會將currTask修改到
				//virtual deadline
				if((GW->TBS.front().target != -1) && (GW->TBS.front().remaining > fogTransfer)){
					GW->TBS.front().deadline = GW->TBS.front().virtualD-fogTransfer;	
				}
				else{
					GW->TBS.front().deadline = GW->TBS.front().virtualD;
				}
			
				GW->remote_q.ready_q.push_back(GW->TBS.front());
				GW->currTask = &GW->remote_q.ready_q.front();
				GW->remote_q.ready_q.pop_front();
				GW->TBS.pop_front();
			}
			debug(("TBS1\r\n"));

			GW->update_U();				// update current total utilization
			battSum += GW->batt;
			utiSum += GW->current_U;
			traffic += GW->Cloud.size();
		}
		
		GW = NodeHead;
		while (GW->nextNode != NULL){
			GW = GW->nextNode;
			//while(timeTick <= HyperPeriod){
			//	
			if((GW->currTask->deadline <= timeTick)&&(GW->currTask->remaining > 0)) {	
				if(GW->currTask->offload){
					debug(("miss_r !\r\n")); Time("miss_r");
					if(GW->currTask->parent != GW->id){
						Node *parent = new Node;
						parent = backMigraSrc(GW->currTask);
						parent->result.miss++;
						GW->currTask->deadline = GW->currTask->virtualD;	// restore origin deadline
						//GW->currTask->deadline += GW->currTask->period;
						//GW->currTask->virtualD = GW->currTask->deadline;
						//GW->currTask->deadline = GW->currTask->virtualD-fogTransfer-(GW->currTask->exec-2*fogTransfer);		// virtual deadline
						//GW->currTask->remaining = GW->currTask->exec;
						//GW->currTask->cnt +=1;
						parent->remote_q.wait_q.push_back(*GW->currTask);
						parent =NULL;
						delete parent;
					}
					else{
						GW->result.miss++;
						GW->currTask->deadline = GW->currTask->virtualD;	// restore origin deadline
						GW->remote_q.wait_q.push_back(*GW->currTask);
					}
				}
				else {
					debug(("miss_l !\r\n")); Time("miss_l");
					GW->result.miss++;
					GW->local_q.wait_q.push_back(*GW->currTask);				
				}
				GW->currTask = idleTask;
			}

			sched_new(GW);				// schedule new task
			debug(("Sched_new !\r\n"));
			
			cludServer(GW);				// cloud computing
			debug(("TimeTick : %d\t GW_%d\t T_%d\r\n", timeTick, GW->id, GW->currTask->id));
			//printSched("");

			// Offloading task
			if(GW->currTask->offload){
				// Evaluate Fog migration target at first running
				if((GW->currTask->remaining == GW->currTask->exec)&&(GW->currTask->target != -1)){
					EvaluationFog(GW->currTask);
				}
				
				GW->currTask->remaining -= GW->speed;
				debug(("--_r !\r\n"));
				GW->result.energy += (p_idle+(float)p_comp/4.0);	// calculate GW offloading energy (proccessing(light loading) + transmission)

				//============ Fog offfloading =============
				if(GW->currTask->target != -1){
					// Migration to Fog device
					if((GW->currTask->remaining <= (GW->currTask->exec-fogTransfer)) && (GW->currTask->remaining > fogTransfer) && (GW->currTask->target != GW->id)){
						debug(("offload_fog !\r\n")); char *state=""; Time("offload_fog");
						GW->result.energy += p_trans*fogTransfer;	// calculate GW offloading energy
						//int temp_D = GW->currTask->deadline;		// store origin deadline
						//GW->currTask->virtualD = GW->currTask->deadline-fogTransfer;
						//GW->currTask->deadline = GW->currTask->virtualD;
						//GW->currTask->virtualD = temp_D;
						GW->currTask->remaining = GW->currTask->exec;
						Migration(GW, findMigraDest(GW->currTask));
					}
					// Migration back
					else if((GW->currTask->remaining <= 0)&&(GW->currTask->parent != GW->id)){
						debug(("back_fog !\r\n")); char *state=""; Time("back_fog");
						GW->result.energy += p_trans*fogTransfer;	// calculate GW offloading energy
						//GW->currTask->deadline = GW->currTask->virtualD;	// restore origin deadline
						GW->currTask->remaining = fogTransfer;
						Migration(GW, backMigraSrc(GW->currTask));
						GW->currTask = idleTask;
					}
					else if(GW->currTask->remaining <= 0){
						debug(("finish_fog !\r\n")); Time("finish_fog");
						GW->result.meet++;
						GW->result.resp += timeTick+1 - (GW->currTask->deadline - GW->currTask->period);
						GW->remote_q.wait_q.push_back(*GW->currTask);
						GW->currTask = idleTask;
					}
					/*else if(GW->currTask->parent != GW->id){
						debug(("Error_fog !\r\n")); Time("error_fog");
					}*/
				}

				//============ Cloud offfloading ===========
				else{
					// offload to cloud
					if(GW->currTask->remaining <= GW->currTask->exec-offloadTransfer && GW->currTask->deadline < GW->currTask->virtualD){
						debug(("offload_cloud !\r\n")); Time("offload_cloud");
						GW->result.energy += p_trans*offloadTransfer;	// calculate GW offloading energy
						GW->currTask->remaining = GW->currTask->exec; 
						GW->Cloud.push_back(*GW->currTask);
						GW->currTask = idleTask;
					}
					// offloading task finish
					else if(GW->currTask->remaining <= 0 && GW->currTask->deadline >= GW->currTask->virtualD){
						debug(("finish_cluod !\r\n")); Time("finish_cloud");
						GW->result.energy += p_trans*offloadTransfer;	// calculate GW offloading energy
						GW->result.meet++;
						GW->result.resp += timeTick+1 - (GW->currTask->deadline - GW->currTask->period);
						GW->remote_q.wait_q.push_back(*GW->currTask);
						GW->currTask = idleTask;
					}
					/*else
						debug(("Error_cloud !\r\n")); Time("error_cluod");*/
				}
			}

			//local task
			else{
				GW->currTask->remaining -= GW->speed;
				debug(("--_l !\r\n"));
				
				GW->result.energy += (GW->currTask->id == idleTask->id)? p_idle : (p_comp+p_idle);  // calculate GW computing energy
				
				if(GW->currTask->remaining <= 0){
					debug(("finish_l !\r\n")); Time("finish_l");
					GW->result.meet++;
					GW->result.resp += timeTick+1 - (GW->currTask->deadline - GW->currTask->period);
					GW->local_q.wait_q.push_back(*GW->currTask);
					GW->currTask = idleTask;
					
				}	
			}
			//timeTick++;
			//idleTask->remaining =9999;
		}
		//GW->result.calculate();		
		debug(("Next !\r\n"));	
		timeTick++;
		idleTask->remaining =9999;
	}
	target = NULL;
	parent = NULL;
	delete target;
	delete parent;
}

void FIFO(){
	timeTick = 0;
	
	while(timeTick<=HyperPeriod+1){
		
		GW = NodeHead;
		traffic = 0;
		debug(("head\r\n"));

		while (GW->nextNode != NULL){
			GW = GW->nextNode;
			debug(("TBS0\r\n"));
			// TBS -> remote_q
			while(!GW->TBS.empty()){
				
				GW->remote_q.ready_q.push_front(*GW->currTask);		// currTask 暫存，不知明原因執行push時會將currTask修改到
				GW->remote_q.ready_q.push_back(GW->TBS.front());
				GW->currTask = &GW->remote_q.ready_q.front();
				GW->remote_q.ready_q.pop_front();
				GW->TBS.pop_front();
			}
			debug(("TBS1\r\n"));
			traffic += GW->Cloud.size();
		}

		GW = NodeHead;
		while(GW->nextNode != NULL){
			GW = GW->nextNode;

			if((GW->currTask->deadline <= timeTick)&&(GW->currTask->remaining > 0)) {	
				if(GW->currTask->offload){
					debug(("miss_r !\r\n")); Time("miss_r");
					GW->result.miss++;
					GW->remote_q.wait_q.push_back(*GW->currTask);
				}
				else {
					debug(("miss_l !\r\n")); Time("miss_l");
					GW->result.miss++;
					GW->local_q.wait_q.push_back(*GW->currTask);				
				}
				GW->currTask = idleTask;
				sched_fifo(GW);				// schedule new task
				debug(("Sched_new !\r\n"));
			}
			else if(GW->currTask->id == idleTask->id){
				sched_fifo(GW);				// schedule new task
				debug(("Sched_new !\r\n"));
			}

			cludServer(GW);				// cloud computing
			debug(("TimeTick : %d\t GW_%d\t T_%d\r\n", timeTick, GW->id, GW->currTask->id));

			// Offloading task
			if(GW->currTask->offload){
				
				GW->currTask->remaining -= GW->speed;
				debug(("--_r !\r\n"));
				GW->result.energy += (p_idle+(float)p_comp/4.0);	// calculate GW offloading energy (proccessing(light loading) + transmission)

				//============ Cloud offfloading ===========
				// offload to cloud
				if(GW->currTask->remaining <= GW->currTask->exec-offloadTransfer && GW->currTask->remaining > offloadTransfer){
					debug(("offload_cloud !\r\n")); Time("offload_cloud");
					GW->result.energy += p_trans*offloadTransfer;	// calculate GW offloading energy
					GW->currTask->remaining = GW->currTask->exec; 
					GW->Cloud.push_back(*GW->currTask);
					GW->currTask = idleTask;
				}
				// offloading task finish
				else if(GW->currTask->remaining <= 0){
					debug(("finish_cluod !\r\n")); Time("finish_cloud");
					GW->result.energy += p_trans*offloadTransfer;	// calculate GW offloading energy
					GW->result.meet++;
					GW->result.resp += timeTick+1 - (GW->currTask->deadline - GW->currTask->period);
					GW->remote_q.wait_q.push_back(*GW->currTask);
					GW->currTask = idleTask;
				}	
			}
			//local task
			else{
				GW->currTask->remaining -= GW->speed;
				debug(("--_l !\r\n"));
				
				GW->result.energy += (GW->currTask->id == idleTask->id)? p_idle : (p_comp+p_idle);  // calculate GW computing energy
				
				if(GW->currTask->remaining <= 0){
					debug(("finish_l !\r\n")); Time("finish_l");
					GW->result.meet++;
					GW->result.resp += timeTick+1 - (GW->currTask->deadline - GW->currTask->period);
					GW->local_q.wait_q.push_back(*GW->currTask);
					GW->currTask = idleTask;
				}	
			}

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
		FIFO();
		break;

	case 2:
		EDF();
		break;
	
	default:
		EDF();
		break;
	}
}

void printSched(char* state) {
	if(GW->id >= 0){
		int t;
		if(state == "context switch" || state == "miss_lq" || state == "miss_rq" || state =="miss_r" || state =="miss_l"){
			t= timeTick;
		}
		else{
			t = timeTick+1; 
		}
		if(GW->currTask->target != -1)
			fs << "TimeTick = " << t << "\tGW_" << GW->id << "\t" << GW->currTask->parent << "_" << GW->currTask->id << "_" << GW->currTask->cnt << "->" << GW->currTask->parent << "_" << GW->currTask->target << "\t" << state << "\t" << GW->currTask->deadline << "\t" << GW->currTask->remaining <<  endl;
		else
			fs << "TimeTick = " << t << "\tGW_" << GW->id << "\t" << GW->currTask->parent << "_" << GW->currTask->id << "_" << GW->currTask->cnt << "\t\t" << state << "\t" << GW->currTask->deadline << "\t" << GW->currTask->remaining << endl;
	}
	
}

void printResult(Node* GW){
	fs << "=========== GW_" << GW->id << " ===========\n";
	fs << "Total Task : " << GW->result.totalTask << endl;
	fs << "Meet Task : " << GW->result.meet << endl;
	fs << "Miss Task : " << GW->result.miss << endl;
	fs << "Meet Ratio : " << GW->result.meet_ratio << endl;
	fs << "Energy Consumption : " << GW->result.energy << endl;
	//fs << "Lifetime : " << GW->result.lifetime << endl;
	
}