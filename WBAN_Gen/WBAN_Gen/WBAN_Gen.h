#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <ctime>
#include <deque>
using namespace std;

/*=================================
		Structure
==================================*/
struct Task{
	int id;
	int period;
	int exec;
	float uti;

	bool offload;
	
	Task(){
		id = 0;
		period = 0;
		exec = 0;
		uti = 0;
		offload = false;
	}
};

struct Node{
	int id;
	float total_U;
	deque<Task> task_q;

	struct Node* nextNode;
	struct Node* preNode;

	Node(){
		id = 0;
		total_U = 0.0;
		task_q.clear();
		nextNode = NULL;
		preNode = NULL;
	}
};

