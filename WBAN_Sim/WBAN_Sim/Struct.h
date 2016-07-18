
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
	float localEng;
	float remoteEng;
	
	Task(){
		id = 0;
		period = 0;
		exec = 0;
		uti = 0;
		offload = false;
		localEng = 0;
		remoteEng = 0;
	}
};

struct Queue{
	deque<Task>ready_q;
	deque<Task>wait_q;

	Queue(){
		ready_q.clear();
		wait_q.clear();
	}

};

struct Node{
	int id;
	float total_U;
	deque<Task> task_q;

	Node* nextNode;
	Node* preNode;

	Queue local_q;		// local queue
	Queue remote_q;		// remote queue

	Node(){
		id = 0;
		total_U = 0.0;
		task_q.clear();
		nextNode = NULL;
		preNode = NULL;
	}
};

