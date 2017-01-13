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

				it->uplink = (it->target == -1)? (offloadTransfer+ceil((float)it->exec/WBANpayload))*_traffic : fogTransfer;
				it->dwlink = (it->target == -1)? (offloadTransfer+ceil((float)it->exec/WBANpayload))*_traffic : fogTransfer;
				it->target = (it->target != -1)? 999 : -1;
				it->vm = -1;
				it->remaining = it->uplink;
				it->cnt += 1;

				if(it->deadline <= HyperPeriod){
					GW->result.totalTask++;
				}

				if(policyOFLD==myOFLD) {
					it->deadline = (it->target != -1)? (it->virtualD-it->dwlink-it->exec) : (it->virtualD-it->dwlink-it->exec/speedRatio);	//virtual deadline
				}

				if(policyOFLD==AOFLDC){
					//it->deadline--;
					it->vm = NodeNum-1;
				}

				GW->remote_q.ready_q.push_front(*GW->currTask);		// currTask 暫存，不知明原因執行push時會將currTask修改到
				GW->remote_q.ready_q.push_back(*it);					// push to ready_q
				GW->currTask = &GW->remote_q.ready_q.front();
				GW->remote_q.ready_q.pop_front();

				it = GW->remote_q.wait_q.erase(it,it+1);		// remove from wait_q
			}
			else {
				++it;									// check next task
			}
		}
	}
	if(!GW->local_q.wait_q.empty()){
		sort(GW->local_q.wait_q.begin(), GW->local_q.wait_q.end(), minDeadline);	// sort by deadline min -> max

		for(deque<Task>::iterator it=GW->local_q.wait_q.begin(); it!=GW->local_q.wait_q.end();){
			if(it->deadline == timeTick){				// task arrival
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

				it = GW->local_q.wait_q.erase(it,it+1);		// remove from wait_q
			}
			else {
				++it;									// check next task
			}
		}
	}

	// sorting task from minDeadline to maxDeadline
	sort(GW->remote_q.ready_q.begin(), GW->remote_q.ready_q.end(), minDeadline);
	sort(GW->local_q.ready_q.begin(), GW->local_q.ready_q.end(), minDeadline);
	
	/*===========================================================
				find minDeadline task to execute
	===========================================================*/
	// remote_q
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
		if((GW->currTask->deadline <= timeTick) && (GW->currTask->remaining > 0)){
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
		else if(GW->currTask->deadline < timeTick + GW->currTask->remaining && policyOFLD==myOFLD){
			GW->currTask->remaining = 0;
			GW->currTask->deadline = GW->currTask->virtualD;
			GW->remote_q.wait_q.push_back(*GW->currTask);
			GW->currTask = idleTask;
			GW->result.miss++;
			sched_new(GW);
		}

	}
	// local_q
	if ((!GW->local_q.ready_q.empty()) && (GW->local_q.ready_q.begin()->deadline < GW->currTask->deadline || (GW->local_q.ready_q.begin()->deadline == GW->currTask->deadline && GW->local_q.ready_q.begin()->offload != GW->currTask->offload))) {	// notice: if deadline was equal, local prio > remote prio
		if(GW->currTask->id != idleTask->id) {	
			if(GW->currTask->offload){
				GW->remote_q.ready_q.push_back(*GW->currTask);
			}
			else {
				GW->local_q.ready_q.push_back(*GW->currTask);
			}
		}

		GW->currTask = &GW->local_q.ready_q.front(); Time("context switch");
		GW->local_q.ready_q.pop_front();

		if((GW->currTask->deadline <= timeTick) && (GW->currTask->remaining > 0)){
			debug(("miss_lq !\r\n")); Time("miss_lq");
			GW->result.miss++;
			GW->local_q.wait_q.push_back(*GW->currTask);
			GW->currTask = idleTask;
			sched_new(GW);
		}
		else if(GW->currTask->deadline < timeTick + GW->currTask->remaining && policyOFLD==myOFLD){
			GW->currTask->remaining = 0;
			GW->currTask->deadline = GW->currTask->virtualD;
			GW->local_q.wait_q.push_back(*GW->currTask);
			GW->currTask = idleTask;
			GW->result.miss++;
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
				it->virtualD = it->deadline;
				it->uplink = (offloadTransfer+ceil((float)it->exec/WBANpayload))*_traffic;
				it->dwlink = (offloadTransfer+ceil((float)it->exec/WBANpayload))*_traffic;
				it->remaining = it->uplink;
				it->cnt += 1;

				if(it->deadline <= HyperPeriod){
					GW->result.totalTask++;
				}

				GW->remote_q.ready_q.push_back(*it);					// push to ready_q

				it = GW->remote_q.wait_q.erase(it);		// remove from wait_q
				
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

			}
			else {
				++it;									// check next task
			}
		}
	}
	// sched_new  prio: remote > local
	
	if(!GW->remote_q.ready_q.empty()){
		GW->currTask = &GW->remote_q.ready_q.front(); Time("context switch");
		GW->remote_q.ready_q.pop_front();
	}
	else if(!GW->local_q.ready_q.empty()){
		GW->currTask = &GW->local_q.ready_q.front(); Time("context switch");
		GW->local_q.ready_q.pop_front();
	}
	else{
		GW->currTask = idleTask;
	}

}


int traffic = 0;

void cludServer(){
	GW = NodeHead;

	while(GW->nextNode != NULL){
		GW = GW->nextNode->nextNode->nextNode;

		if(!GW->Cloud.empty()){
			// miss check
			for(deque<Task>::iterator it=GW->Cloud.begin(); it!=GW->Cloud.end();){
				if(it->deadline <= timeTick && it->remaining > 0){
					Time("cloud_miss");
					Node *src = new Node;
					src = NodeHead;
					while(src->nextNode != NULL){
						src = src->nextNode;
						if(src->id == it->parent){
							src->remote_q.wait_q.push_back(*it);
							break;
						}
					}
					GW->result.miss++;

					it = GW->Cloud.erase(it,it+1);
				}
				else{
					++it;
				}
			}
		}

		if(!GW->Cloud.empty()){
			sort(GW->Cloud.begin(), GW->Cloud.end(), minDeadline);	// sort by deadline min -> max
				
			GW->Cloud.front().remaining -= speedRatio; debug(("cloud\r\n"));
			GW->result.serverEng += cloudp_actv;

			if(GW->Cloud.front().remaining <= 0){
				GW->Cloud.front().deadline = GW->Cloud.front().virtualD;
				GW->Cloud.front().remaining = GW->Cloud.front().dwlink;

				Node *src = new Node;
				src = NodeHead;
				while(src->nextNode != NULL){
					src = src->nextNode;
					if(src->id == GW->Cloud.front().parent){
						Time("cloud_back");
						src->TBS.push_back(GW->Cloud.front());
						break;
					}
				}
				GW->Cloud.pop_front();
				src = NULL;
				delete src;
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

void EvaluationVM(Task *task){
	target = NodeHead;

	while(target->nextNode != NULL){
		target = target->nextNode->nextNode->nextNode;
		target->cloudADM(task->exec, task->virtualD-task->dwlink, task->uplink);
	}

	target = NodeHead;
	float temp_CCadm = INT_MAX;
	while (target->nextNode != NULL) {
		target = target->nextNode->nextNode->nextNode;
		//cout << target->CCadm << ", "; 
		if((target->CCadm < temp_CCadm) && (target->CCadm > 0)){
			task->vm = target->id;
		}
	}

	if(task->vm == -1){
		task->vm = 999;
	}

	//cout << endl;
	target = NULL;

}

void to_Cloud(Task *task){
	target = NodeHead;
	while(target->nextNode != NULL){
		target = target->nextNode;
		if(target->id == task->vm){
			target->Cloud.push_back(*task);
		}
	}
}
//============================== Evaluate migration target ============================
void EvaluationFog(Task *task){
	target = NodeHead;

	while(target->nextNode != NULL){
		target = target->nextNode;
		target->MW(task->localEng, utiSum, task->deadline);
		target->ADM(task->exec, task->virtualD-task->dwlink, task->uplink);
		//printf("(%d,%f,%d,%f)\t",target->batt,target->current_U,target->block,target->migratWeight);
	}
	
	target = NodeHead;
	float temp_MW = 0;
	float temp_U = target->nextNode->current_U;
	float temp_ADM = target->nextNode->admin;
	while (target->nextNode != NULL) {
		target = target->nextNode;
		//cout << target->admin << "," << target->current_U << "\t"; 
		//if((target->id != task->parent) && ((target->admin < temp_ADM) || (target->admin == temp_ADM && target->current_U < temp_U)) && (target->admin > 0) && (1.0-target->current_U >= task->exec/target->speed/(task->virtualD-task->dwlink)) && (target->current_U <= 1.0)){
		if((target->admin < task->period-task->dwlink-task->uplink) && (target->admin > 0) && (1.0-target->current_U >= task->exec/target->speed/(task->virtualD-task->dwlink)) && (target->current_U <= 1.0)){
			if(temp_MW == 0){
				temp_MW = target->migratWeight;
				task->target = target->id;
			}
			else if(target->migratWeight > temp_MW){
				task->target = target->id;
				temp_MW = target->migratWeight;
			}
		}
	}
	
	//cout << endl;
	// can not find proper dest >> local running
	if(task->target == 999 || task->target == task->parent){
		task->target = task->parent;
		task->deadline = task->virtualD;	// original deadline
		task->remaining = task->exec;
	}
	else if(task->target == 999){
		task->remaining = 0;
		task->deadline = task->virtualD;
		GW->remote_q.wait_q.push_back(*GW->currTask);
		GW->currTask = idleTask;
		GW->result.miss++;
	}
	else{
		target = NodeHead;
		while (target->nextNode != NULL){
			target = target->nextNode;
			if(target->id == task->target)
				target->remaingFog += task->exec/target->speed;
		}
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
		if(timeTick == 300){
			int a =1;
		}
		GW = NodeHead;
		battSum = 0;
		utiSum = 0.0;
		traffic = 0;
		debug(("head\r\n"));

		cludServer();				// cloud computing
		debug(("TimeTick : %d\t GW_%d\t T_%d\r\n", timeTick, GW->id, GW->currTask->id));
		
		GW = NodeHead;
		while (GW->nextNode != NULL){
			GW = GW->nextNode;
			debug(("TBS0\r\n"));
			// TBS -> remote_q
			while(!GW->TBS.empty()){
				
				GW->remote_q.ready_q.push_front(*GW->currTask);		// currTask 暫存，不知明原因執行push時會將currTask修改到
				//virtual deadline
				if((GW->TBS.front().target != -1) && (GW->TBS.front().parent != GW->id)){ // phase 2 deadline
					GW->TBS.front().deadline = GW->TBS.front().virtualD; //- GW->TBS.front().dwlink;	
				}
				else{
					GW->TBS.front().deadline = GW->TBS.front().virtualD; // phase 3 origin deadline
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

			if((GW->currTask->deadline <= timeTick) && (GW->currTask->remaining > 0)) {	
				if(GW->currTask->offload){
					debug(("miss_r !\r\n")); Time("miss_r");
					if(GW->currTask->parent != GW->id){						// task not belong to this GW
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

			/*******************************************
							Offfloading
			*******************************************/
			if(GW->currTask->offload){
				
				if(policyOFLD == myOFLD){
					if(GW->currTask->target == -1 && GW->currTask->vm == -1){
						EvaluationVM(GW->currTask);
					}

					// Evaluate Fog migration target at first running
					if(GW->currTask->target == 999 || GW->currTask->vm == 999){
						EvaluationFog(GW->currTask);
					}
				}
				

				debug(("--_r !\r\n"));
				//GW->result.energy += (p_idle+(float)p_comp/4.0);	// calculate GW offloading energy (proccessing(light loading) + transmission)

				/*=========================================
							  Fog offfloading
				==========================================*/
				if(GW->currTask->target != -1){
					// pre-processing
					if((GW->currTask->uplink > 0) && (GW->currTask->target != GW->id)){
						
						GW->currTask->uplink--;
						GW->currTask->remaining = GW->currTask->uplink;
						if(GW->currTask->uplink > GW->currTask->dwlink-(proc-1)){
							GW->result.energy += p_idle + p_comp;	// calculate GW offloading energy
						}
						else{
							GW->result.energy += p_idle + p_trans;	// calculate GW offloading energy
						}
						//Migration to Fog device
						if(GW->currTask->uplink <= 0){
							debug(("offload_fog !\r\n")); char *state=""; Time("offload_fog");
							GW->currTask->remaining = GW->currTask->exec;
							GW->currTask->deadline = GW->currTask->virtualD; //- GW->currTask->dwlink;	//phase 2 D
							Migration(GW, findMigraDest(GW->currTask));
							GW->currTask = idleTask;
						}
					}
					// fog-processing
					else if((GW->currTask->uplink <= 0) && (GW->currTask->dwlink > 0) && (GW->currTask->parent != GW->id)){
						
						GW->currTask->remaining -= GW->speed;
						GW->result.energy += p_idle + p_comp;	// calculate GW offloading energy
						//Migration back 
						if(GW->currTask->remaining <= 0){
							debug(("back_fog !\r\n")); char *state=""; Time("back_fog");
							GW->currTask->deadline = GW->currTask->virtualD;	// phase 3 origin D
							GW->currTask->remaining = GW->currTask->dwlink;
							GW->remaingFog -= GW->currTask->exec;
							Migration(GW, backMigraSrc(GW->currTask));
							GW->currTask = idleTask;	
						}
					}
					// post-processing
					else if((GW->currTask->uplink <= 0) && (GW->currTask->dwlink > 0) && (GW->currTask->parent == GW->id)){
						
						GW->currTask->dwlink--;
						GW->currTask->remaining = GW->currTask->dwlink;
						if(GW->currTask->dwlink < proc-1){
							GW->result.energy += p_idle + p_comp;	// calculate GW offloading energy
						}
						else{
							GW->result.energy += p_idle + p_trans;	// calculate GW offloading energy
						}
						
						if(GW->currTask->dwlink <= 0){
							debug(("finish_fog !\r\n")); Time("finish_fog");
							GW->result.meet++;
							GW->result.resp += timeTick+1 - (GW->currTask->deadline-GW->currTask->period);
							GW->remote_q.wait_q.push_back(*GW->currTask);
							GW->currTask = idleTask;
						}
					}
					else if(GW->currTask->target == GW->id){
						GW->currTask->remaining -= GW->speed;
						debug(("--_l_fog !\r\n"));
				
						GW->result.energy += (p_comp + p_idle);  // calculate GW computing energy
				
						if(GW->currTask->remaining <= 0){
							debug(("finish_l_fog !\r\n")); Time("finish_l_fog");
							GW->result.meet++;
							GW->result.resp += timeTick+1 - (GW->currTask->deadline-GW->currTask->period);
							GW->remote_q.wait_q.push_back(*GW->currTask);
							GW->currTask = idleTask;
						}	
					}
					else{
						Time("Error");
					}
				}

				/*=========================================
							  Cloud offfloading
				==========================================*/
				else{
					if(GW->currTask->vm == 999){
						GW->ADM(GW->currTask->exec, GW->currTask->virtualD, 0);
						if(GW->admin-GW->currTask->remaining <= GW->currTask->virtualD-timeTick && 1.0-GW->current_U > GW->currTask->uti){
							GW->currTask->vm = -999;
							GW->currTask->deadline = GW->currTask->virtualD;
							GW->currTask->remaining = GW->currTask->exec;
							GW->currTask->remaining -= GW->speed;
							debug(("--_l_cloud !\r\n"));
							GW->result.energy += (p_comp + p_idle);  // calculate GW computing energy
						}
						else{
							GW->currTask->deadline = GW->currTask->virtualD;
							Time("VM miss");
							GW->result.miss++;
							GW->remote_q.wait_q.push_back(*GW->currTask);
							GW->currTask = idleTask;
						}
					}
					else if(GW->currTask->vm == -999){
						GW->currTask->remaining -= GW->speed;
						debug(("--_l_cloud !\r\n"));
				
						GW->result.energy += (p_comp + p_idle);  // calculate GW computing energy
				
						if(GW->currTask->remaining <= 0){
							debug(("finish_l_cloud !\r\n")); Time("finish_l_cloud");
							GW->result.meet++;
							GW->result.resp += timeTick+1 - (GW->currTask->deadline-GW->currTask->period);
							GW->remote_q.wait_q.push_back(*GW->currTask);
							GW->currTask = idleTask;
						}
					}

					
					// offload to cloud
					else if(GW->currTask->uplink > 0){

						GW->currTask->uplink--;
						GW->currTask->remaining = GW->currTask->uplink;
						if(GW->currTask->uplink > GW->currTask->dwlink-proc){
							GW->result.energy += p_idle + p_comp;	// calculate GW offloading energy
						}
						else{
							GW->result.energy += p_idle + p_trans;	// calculate GW offloading energy
						}

						if(GW->currTask->uplink <= 0){
							debug(("offload_cloud !\r\n")); Time("offload_cloud");
							GW->currTask->remaining = GW->currTask->exec;
							if(policyOFLD == myOFLD){
								GW->currTask->deadline = GW->currTask->virtualD - GW->currTask->dwlink;	// phase 2 D
							}
							to_Cloud(GW->currTask);
							GW->currTask = idleTask;
						}
					}
					// offloading task finish
					else if((GW->currTask->uplink <=0) && (GW->currTask->dwlink > 0)){

						GW->currTask->dwlink--;
						GW->currTask->remaining = GW->currTask->dwlink;
						if(GW->currTask->dwlink < proc){
							GW->result.energy += p_idle + p_comp;	// calculate GW offloading energy
						}
						else{
							GW->result.energy += p_idle + p_trans;	// calculate GW offloading energy
						}

						if(GW->currTask->dwlink <= 0){
							debug(("finish_cluod !\r\n")); Time("finish_cloud");
							GW->currTask->vm = -1;
							GW->result.meet++;
							GW->result.resp += timeTick+1 - (GW->currTask->deadline-GW->currTask->period);
							GW->remote_q.wait_q.push_back(*GW->currTask);
							GW->currTask = idleTask;
						}
					}
					else{
						Time("Error");
					}
				}
			}

			/*******************************************
								Local
			*******************************************/
			else{
				GW->currTask->remaining -= GW->speed;
				debug(("--_l !\r\n"));
				
				GW->result.energy += (GW->currTask->id == idleTask->id)? p_idle : (p_comp+p_idle);  // calculate GW computing energy
				
				if(GW->currTask->remaining <= 0){
					debug(("finish_l !\r\n")); Time("finish_l");
					GW->result.meet++;
					GW->result.resp += timeTick+1 - (GW->currTask->deadline-GW->currTask->period);
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
		
		traffic = 0;
		debug(("head\r\n"));

		cludServer();				// cloud computing
		debug(("TimeTick : %d\t GW_%d\t T_%d\r\n", timeTick, GW->id, GW->currTask->id));
		
		GW = NodeHead;
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

			if((GW->currTask->deadline <= timeTick) && (GW->currTask->remaining > 0)) {	
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

			/*******************************************
							Offfloading
			*******************************************/
			if(GW->currTask->offload){
				
				if(GW->currTask->target == -1 && GW->currTask->vm == -1){
					//EvaluationVM(GW->currTask);
					GW->currTask->vm = NodeNum-1;
				}

				debug(("--_r !\r\n"));

				//============ Cloud offfloading ===========
				// offload to cloud
				if(GW->currTask->uplink > 0){
					GW->currTask->uplink--;
					GW->currTask->remaining = GW->currTask->uplink;
					if(GW->currTask->uplink > GW->currTask->dwlink-proc){
							GW->result.energy += p_idle + p_comp;	// calculate GW offloading energy
						}
						else{
							GW->result.energy += p_idle + p_trans;	// calculate GW offloading energy
						}

					if(GW->currTask->uplink <= 0){
						debug(("offload_cloud !\r\n")); Time("offload_cloud");
						GW->currTask->remaining = GW->currTask->exec; 
						to_Cloud(GW->currTask);
						GW->currTask = idleTask;
					}
				}
				// offloading task finish
				else if((GW->currTask->uplink <=0) && (GW->currTask->dwlink > 0)){
					GW->currTask->dwlink--;
					GW->currTask->remaining = GW->currTask->dwlink;
					if(GW->currTask->dwlink < proc){
							GW->result.energy += p_idle + p_comp;	// calculate GW offloading energy
						}
					else{
							GW->result.energy += p_idle + p_trans;	// calculate GW offloading energy
						}

					if(GW->currTask->dwlink <= 0){
						debug(("finish_cluod !\r\n")); Time("finish_cloud");
						GW->currTask->vm = -1;
						GW->result.meet++;
						GW->result.resp += timeTick+1 - (GW->currTask->deadline-GW->currTask->period);
						GW->remote_q.wait_q.push_back(*GW->currTask);
						GW->currTask = idleTask;
					}
				}
				else {
					Time("Error");
				}
			}
			/*******************************************
								Local
			*******************************************/
			else{
				GW->currTask->remaining -= GW->speed;
				debug(("--_l !\r\n"));
				
				GW->result.energy += (GW->currTask->id == idleTask->id)? p_idle : (p_comp+p_idle);  // calculate GW computing energy
				
				if(GW->currTask->remaining <= 0){
					debug(("finish_l !\r\n")); Time("finish_l");
					GW->result.meet++;
					GW->result.resp += timeTick+1 - (GW->currTask->deadline-GW->currTask->period);
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
		if(state == "context switch" || state == "miss_lq" || state == "miss_rq" || state =="miss_r" || state =="miss_l" || state == "cloud_miss" || state == "cloud_back"){
			t= timeTick;
		}
		else{
			t = timeTick+1; 
		}
		if(GW->currTask->target != -1)
			fs << "TimeTick = " << t << "\tGW_" << GW->id << "\t" << GW->currTask->parent << "_" << GW->currTask->id << "_" << GW->currTask->cnt << "->" << GW->currTask->parent << "_" << GW->currTask->target << "\t" << state << "\t" << GW->currTask->deadline << "\t" << GW->currTask->remaining <<  endl;
		else if(state == "cloud_miss" || state == "cloud_back")
			fs << "TimeTick = " << t << "\tGW_" << GW->id << "\t" << GW->Cloud.front().parent << "_" << GW->Cloud.front().id << "_" << GW->Cloud.front().cnt << "\t\t" << state << "\t" << GW->Cloud.front().deadline << "\t" << GW->Cloud.front().remaining << endl;
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