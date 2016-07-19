#include <iostream>
#include <iomanip>
#include <deque>
#include <vector>
#include <fstream>

#include "Struct.h"
#include "Dispatch.h"

using namespace std;

void q_init(Node* GW){
	
	GW->local_q.ready_q.clear();
	GW->local_q.wait_q.clear();

	GW->remote_q.ready_q.clear();
	GW->remote_q.wait_q.clear();

	GW->total_U = 0;
}

void setPrio(Task task){
	
	task.prio = 1;
}

void dispatch(Node* GW){
	
	q_init(GW);

	for(deque<Task>::iterator it=GW->task_q.begin(); it!=GW->task_q.end(); ++it){
		if(it->offload == true){
			setPrio(*it);
			GW->remote_q.ready_q.push_back(*it);
		}
		else{
			setPrio(*it);
			GW->local_q.ready_q.push_back(*it);
		}
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