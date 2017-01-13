#include <iostream>
#include <iomanip>
#include <string>
#include <ctime>
#include <deque>
#include <vector>
#include <algorithm>
#include <fstream>

#include "Struct.h"
#include "Gen.h"
#include "Debug.h"
using namespace std;

#define DEBUG 1

#ifdef DEBUG
#define debug(x) printf x
#endif

/*======== subfunction ============*/
bool cmp(float i, float j) {return i > j;}

/*=================================
		Construct Node & Task
==================================*/
void Create(){
	Clear();
	NodeHead = new Node;
	NodeHead->nextNode = NULL;
	NodeHead->preNode = NodeHead;

	/*=========================== Gen GW ============================*/
	for(int n =0; n<NodeNum; ++n){
		GW = new Node;
		GW->preNode = NodeHead->preNode;
		GW->nextNode = NULL;
		GW->id = n;
		GW->currTask = new Task;
		GW->currTask = idleTask;
		NodeHead->preNode->nextNode = GW;
		NodeHead->preNode = GW;

		/*---------------- Gen task -----------------*/
		
		// last GW as a fog server without any tasks (NodeNum-1)
		if(GW->id >= NodeNum-fogserver-1){
			taskgen = new Task;
			GW->task_q.push_back(*taskgen);
		}
		else{
			for(int t=0; t<TaskNum; ++t){
				taskgen = new Task;
				GW->task_q.push_back(*taskgen);
			}
		}

	}

	idleTask->id = 999;
	idleTask->period = 9999;
	idleTask->deadline = 9999;
	idleTask->exec = 9999;
	idleTask->remaining =9999;
	idleTask->offload = false;
	idleTask->_setPrio(999);
	idleTask->uti = 0;
}

/*=================================
		Generate Node & Task
==================================*/
void WBAN_Gen(){
	int x = rand()% sizeof(*period); // !!!!!!!!!!!!!!!!!!!!!! period all the same, just for resp test
	GW = NodeHead;
	while (GW->nextNode != NULL){
		GW = GW->nextNode;

		// last GW as a fog server without any tasks
		if(GW->id >= NodeNum-fogserver)
			break;

		float remain_U = total_U;
		//GW->batt = total_U;
		// last GW as a fog server with light load
		if(GW->id == NodeNum-fogserver-1){
			remain_U = 0.3;
			///GW->batt = 0.25;
			// set task parameters to node
			GW->task_q.at(0).id = 0;										// task id
			GW->task_q.at(0).cnt = 0;										// task counter
			GW->task_q.at(0).parent = GW->id;								// GW id
			GW->task_q.at(0).period = 1000;								// task period
			GW->task_q.at(0).deadline = 1000;									// task deadline
			GW->task_q.at(0).exec = 0.3 *1000* GW->speed;						// task execution
			GW->task_q.at(0).remaining = 0.3 * 1000 *GW->speed;					// task remaining time
			GW->task_q.at(0).uti = 0.3;	// task utilization
			GW->total_U += GW->task_q.at(0).uti;							// node total utilization
			break;
		}
		
		vector<float> U;
		vector<int> P;
		for(int t=0; t<TaskNum; ++t){
			float uti = (t==TaskNum-1) ? remain_U : (float)rand()/RAND_MAX;						// the last task uti = remain uti
			while((uti<lowest_U || uti>remain_U || uti>remain_U-(TaskNum-t-1)*lowest_U || uti>total_U-(TaskNum-t-1)*lowest_U || uti>1.0)&&(t!=TaskNum-1)){		// uti > 0.05
				uti = ((remain_U-(TaskNum-t-1)*lowest_U)-lowest_U+1)*(float)rand()/(RAND_MAX) + lowest_U;
			}
			//P.push_back(period[x]); // !!!!!!!!!!!!!!!!!!!!!! period all the same, just for resp test
			P.push_back(period[rand()% sizeof(*period)]);
			U.push_back(uti);
			remain_U -= uti;
		}
		// shorter period has larger utilization
		//sort(P.begin(),P.end());
		sort(U.begin(),U.end(),cmp);

		// set task parameters to node
		for(int t=0; t<TaskNum; ++t){
			GW->task_q.at(t).id = t;										// task id
			GW->task_q.at(t).cnt = 0;										// task counter
			GW->task_q.at(t).parent = GW->id;								// GW id
			GW->task_q.at(t).period = P.at(t);								// task period
			GW->task_q.at(t).deadline = P.at(t);									// task deadline
			GW->task_q.at(t).exec = U.at(t) * P.at(t);						// task execution
			GW->task_q.at(t).remaining = U.at(t) * P.at(t);					// task remaining time
			GW->task_q.at(t).uti = (float)GW->task_q.at(t).exec/P.at(t);	// task utilization
			GW->total_U += GW->task_q.at(t).uti;							// node total utilization
		}
		///total_U = 0.75;
	}
	///total_U = 1.0;
}

/*=================================
  Load Node & Task from input.txt
==================================*/
void WBAN_Load(){
	
	GW = NodeHead;
	char inputBuf[50];
	char* value = (char*)malloc(50);
	int taskID = 0;
	input.clear();
	while (!input.eof()) {
		input.getline(inputBuf, sizeof(inputBuf));
		value = strtok(inputBuf, " ");
		if(strcmp(value, "GW") == 0){
			GW = GW->nextNode;
			taskID = 0;
		}
		if(strcmp(value, "Task") == 0){
			GW->task_q.at(taskID).id = taskID;								// task id
			GW->task_q.at(taskID).cnt = 0;									// task counter
			GW->task_q.at(taskID).parent = GW->id;							// GW id
			
			value = strtok(NULL, " ");
			GW->task_q.at(taskID).exec = atoi(value);						// task execution
			GW->task_q.at(taskID).remaining = atoi(value);					// task remaining time

			value = strtok(NULL, " ");
			GW->task_q.at(taskID).period = atoi(value);						// task period
			GW->task_q.at(taskID).deadline = atoi(value);					// task deadline
			
			GW->task_q.at(taskID).uti = (float)GW->task_q.at(taskID).exec/GW->task_q.at(taskID).period;	// task utilization
			GW->total_U += GW->task_q.at(taskID).uti;						// node total utilization
			taskID++;
		}
		if(strcmp(value, "-------------") == 0){
			input.getline(inputBuf, sizeof(inputBuf));
			input.getline(inputBuf, sizeof(inputBuf));
			break;
		}
	}
	
}
/*=================================
		Clear all nodes
==================================*/
void Clear(){
	GW = NodeHead;
	while(GW->nextNode!=NULL){
		GW->task_q.~deque();
		GW = GW->nextNode;
		delete GW->preNode;	// first time would delete NodeHead
	}
	GW->task_q.~deque();
	delete GW;
}
/*=================================
		Print Gen Result
==================================*/
void Print_WBAN(){
	GW = NodeHead->nextNode;
	while (GW != NULL){
		cout << "Gateway Node : "<< GW->id << "\tU = " << GW->total_U << endl;
		fs << "GW " << GW->total_U << endl;
		for(deque<Task>::iterator it=GW->task_q.begin(); it!=GW->task_q.end(); ++it){
			cout << "\t T" << it->id << " = (" << it->exec << ", " << it->period << ", " << it->uti << ")\n";
			fs << "Task "<< it->exec << " " << it->period << " " << it->uti << "\n";
		}
		cout<<endl;
		GW = GW->nextNode;
	}
	cout<<endl;
}
/*=================================
		Output Gen Result
==================================*/

void Output_WBAN(){
	GW = NodeHead->nextNode;
	while (GW != NULL){
		cout << "Gateway Node : "<< GW->id << "\tU = " << GW->total_U << endl;
		input << "GW " << GW->total_U << endl;
		for(deque<Task>::iterator it=GW->task_q.begin(); it!=GW->task_q.end(); ++it){
			cout << "\t T" << it->id << " = (" << it->exec << ", " << it->period << ", " << it->uti << ")\n";
			input << "Task "<< it->exec << " " << it->period << " " << it->uti << "\n";
		}
		cout<<endl;
		GW = GW->nextNode;
	}
	cout<<endl;
}
