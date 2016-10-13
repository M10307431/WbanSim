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
				
				if(timeTick%HyperPeriod != 0){
					GW->result.totalTask++;
					it->cnt += 1;
				}

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
				
				if(timeTick%HyperPeriod != 0){
					GW->result.totalTask++;
					it->cnt += 1;
				}
				
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
	//debug(("TBS0\r\n"));
	//// TBS -> remote_q
	//for(deque<Task>::iterator it=GW->TBS.begin(); it!=GW->TBS.end();++it){
	//	GW->remote_q.ready_q.push_back(*it);
	//}
	//GW->TBS.clear();
	//debug(("TBS1\r\n"));

	// sorting task from minDeadline to maxDeadline
	sort(GW->remote_q.ready_q.begin(), GW->remote_q.ready_q.end(), minDeadline);
	sort(GW->local_q.ready_q.begin(), GW->local_q.ready_q.end(), minDeadline);
	
	// find minDeadline task to execute
	if((!GW->remote_q.ready_q.empty()) && (GW->remote_q.ready_q.begin()->deadline < GW->currTask->deadline)) {   
		if(GW->currTask != idleTask) {	
			GW->remote_q.ready_q.push_back(*GW->currTask);
		}
		GW->currTask = &GW->remote_q.ready_q.front(); Time("context switch"); // get the min deadline task
		GW->remote_q.ready_q.pop_front();
		
		// not finish task
		if((GW->currTask->deadline < timeTick)&&(GW->currTask->remaining > 0)){
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
	else if ((!GW->local_q.ready_q.empty()) && (GW->local_q.ready_q.begin()->deadline < GW->currTask->deadline) && (!GW->currTask->offload)) {
		if(GW->currTask != idleTask) {	
			GW->local_q.ready_q.push_back(*GW->currTask);
		}
		GW->currTask = &GW->local_q.ready_q.front(); Time("context switch");
		GW->local_q.ready_q.pop_front();
		if((GW->currTask->deadline < timeTick)&&(GW->currTask->remaining > 0)){
			debug(("miss_lq !\r\n")); Time("miss_lq");
			GW->result.miss++;
			GW->local_q.wait_q.push_back(*GW->currTask);
			GW->currTask = idleTask;
			sched_new(GW);
		}	
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
	    Migration
=====================*/
Node *target = new Node;
Node *parent = new Node;

//============================== Evaluate migration target ============================
void EvaluationFog(Task *task){
	
	float temp_U = 1.0;
	target = NodeHead;
	while (target->nextNode != NULL) {
		target = target->nextNode;
		if((target->id != task->parent)&&(target->current_U < temp_U)&&(1.0-target->current_U > task->uti)){
			task->target = target->id;
			task->virtualD = (task->remaining-2*fogTransfer)/(1.0-target->current_U);
			if(task->virtualD > task->deadline-fogTransfer ) {
				task->virtualD = task->deadline-fogTransfer;
			}
		}
	}
	// can not find proper dest >> local running
	if(task->target == 999){
		task->target = task->parent;
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
	
	while(timeTick <= HyperPeriod){
		GW = NodeHead;
		debug(("head\r\n"));

		while (GW->nextNode != NULL){
			GW = GW->nextNode;
			debug(("TBS0\r\n"));
			// TBS -> remote_q
			for(deque<Task>::iterator it=GW->TBS.begin(); it!=GW->TBS.end();++it){
				GW->remote_q.ready_q.push_front(*it);
			}
			GW->TBS.clear();
			debug(("TBS1\r\n"));
			GW->update_U();				// update current total utilization
		}
		
		GW = NodeHead;
		while (GW->nextNode != NULL){
			GW = GW->nextNode;
			//while(timeTick <= HyperPeriod){
			//	
			if((GW->currTask->deadline < timeTick)&&(GW->currTask->remaining > 0)) {	
				if(GW->currTask->offload){
					debug(("miss_r !\r\n")); Time("miss_r");
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
				
				GW->currTask->remaining--;
				debug(("--_r !\r\n"));
				GW->result.energy += (p_idle+(float)p_comp/4.0);	// calculate GW offloading energy (proccessing(light loading) + transmission)

				//============ Fog offfloading =============
				if(GW->currTask->target != -1){
					// Migration to Fog device
					if((GW->currTask->remaining <= (GW->currTask->exec-fogTransfer)) && (GW->currTask->remaining > fogTransfer) && (GW->currTask->target != GW->id)){
						debug(("offload_fog !\r\n")); char *state=""; Time("offload_fog");
						GW->result.energy += p_trans*fogTransfer;	// calculate GW offloading energy
						int temp_D = GW->currTask->deadline;		// store origin deadline
						GW->currTask->virtualD = GW->currTask->deadline-fogTransfer;
						GW->currTask->deadline = GW->currTask->virtualD;
						GW->currTask->virtualD = temp_D;
						Migration(GW, findMigraDest(GW->currTask));
					}
					// Migration back
					else if((GW->currTask->remaining <= fogTransfer)&&(GW->currTask->parent != GW->id)){
						debug(("back_fog !\r\n")); char *state=""; Time("back_fog");
						GW->result.energy += p_trans*fogTransfer;	// calculate GW offloading energy
						GW->currTask->deadline = GW->currTask->virtualD;	// restore origin deadline
						Migration(GW, backMigraSrc(GW->currTask));
						GW->currTask = idleTask;
					}
					else if(GW->currTask->remaining <= 0){
						debug(("finish_fog !\r\n")); Time("finish_fog");
						GW->result.meet++;
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
					if(GW->currTask->remaining <= GW->currTask->exec-offloadTransfer && GW->currTask->remaining > offloadTransfer){
						debug(("offload_cloud !\r\n")); Time("offload_cloud");
						GW->result.energy += p_trans*offloadTransfer;	// calculate GW offloading energy
						GW->Cloud.push_back(*GW->currTask);
						GW->currTask = idleTask;
					}
					// offloading task finish
					else if(GW->currTask->remaining <= 0){
						debug(("finish_cluod !\r\n")); Time("finish_cloud");
						GW->result.energy += p_trans*offloadTransfer;	// calculate GW offloading energy
						GW->result.meet++;
						GW->remote_q.wait_q.push_back(*GW->currTask);
						GW->currTask = idleTask;
					}
					/*else
						debug(("Error_cloud !\r\n")); Time("error_cluod");*/
				}
			}

			//local task
			else{
				GW->currTask->remaining--;
				debug(("--_l !\r\n"));
				
				GW->result.energy += (GW->currTask == idleTask)? p_idle : (p_comp+p_idle);  // calculate GW computing energy
				
				if(GW->currTask->remaining <= 0){
					debug(("finish_l !\r\n")); Time("finish_l");
					GW->result.meet++;
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

void printSched(char* state) {
	if(GW->id >= 0){
		if(GW->currTask->target != -1)
			fs << "TimeTick = " << timeTick << "\tGW_" << GW->id << "\t" << GW->currTask->parent << "_" << GW->currTask->id << "_" << GW->currTask->cnt << "->" << GW->currTask->parent << "_" << GW->currTask->target << "\t" << state << "\t" << GW->currTask->deadline << "\t" << GW->currTask->remaining <<  endl;
		else
			fs << "TimeTick = " << timeTick << "\tGW_" << GW->id << "\t" << GW->currTask->parent << "_" << GW->currTask->id << "_" << GW->currTask->cnt << "\t\t" << state << "\t" << GW->currTask->deadline << "\t" << GW->currTask->remaining << endl;
	}
	
}

void printResult(Node* GW){
	fs << "=========== GW_" << GW->id << " ===========\n";
	fs << "Total Task : " << GW->result.totalTask << endl;
	fs << "Meet Task : " << GW->result.meet << endl;
	fs << "Miss Task : " << GW->result.miss << endl;
	fs << "Meet Ratio : " << GW->result.meet_ratio << endl;
	fs << "Energy Consumption : " << GW->result.energy << endl;
	fs << "Lifetime : " << GW->result.lifetime << endl;
	
}