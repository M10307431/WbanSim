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
using namespace std;

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
		NodeHead->preNode->nextNode = GW;
		NodeHead->preNode = GW;

		/*---------------- Gen task -----------------*/
		for(int t=0; t<TaskNum; ++t){
			taskgen = new Task;
			GW->task_q.push_back(*taskgen);
		}
	}
}

/*=================================
		Generate Node & Task
==================================*/
void WBAN_Gen(){
	GW = NodeHead;
	while (GW->nextNode != NULL){
		GW = GW->nextNode;
		float remain_U = total_U;
		vector<float> U;
		vector<int> P;
		for(int t=0; t<TaskNum; ++t){
			float uti = (t==TaskNum-1) ? remain_U : rand()/RAND_MAX;						// the last task uti = remain uti
			while(uti<lowest_U || uti>remain_U || uti>remain_U-(TaskNum-t-1)*lowest_U)		// uti > 0.01
				uti = (float)rand()/RAND_MAX;
			
			P.push_back(period[rand()% sizeof(*period)]);
			U.push_back(uti);
			remain_U -= uti;
		}
		// shorter period has larger utilization
		sort(P.begin(),P.end());
		sort(U.begin(),U.end(),cmp);
		// set task parameters to node
		for(int t=0; t<TaskNum; ++t){
			GW->task_q.at(t).id = t;										// task id
			GW->task_q.at(t).period = P.at(t);								// task period
			GW->task_q.at(t).exec = U.at(t) * P.at(t);						// task execution
			GW->task_q.at(t).uti = (float)GW->task_q.at(t).exec/P.at(t);	// task utilization
			GW->total_U += GW->task_q.at(t).uti;							// node total utilization
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
	fs << NodeNum << " " << TaskNum << endl;
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
