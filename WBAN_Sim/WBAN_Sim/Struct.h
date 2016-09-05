
using namespace std;

/*=================================
		Structure
==================================*/
struct Task{
	int id;
	int cnt;
	int period;
	int deadline;
	int exec;
	int remaining;
	float uti;
	int prio;

	bool offload;
	float localEng;
	float remoteEng;
	
	void _setPrio(int _prio){
		prio = _prio;
	};

	Task(){
		id = 0;
		cnt = 0;
		period = 0;
		deadline = 0;
		exec = 0;
		remaining = 0;
		uti = 0;
		prio = 0;
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

struct Result{
	double energy;		// (W * sec)
	int totalTask;
	int meet;
	int miss;
	double meet_ratio;
	double lifetime;	// [CR2032] 230mA / energy (hr)


	void clear(){
		energy = 0;
		totalTask = 0;
		meet = 0;
		miss = 0;
		meet_ratio = 0;
		lifetime = 0;
	};

	void calculate(){
		meet_ratio = (float)meet / totalTask;
		lifetime = 0.23*60*60 / energy;
	}
};

struct Node{
	int id;
	float total_U;
	Task* currTask;
	deque<Task> task_q;
	Result result;

	Node* nextNode;
	Node* preNode;

	Queue local_q;		// local queue
	Queue remote_q;		// remote queue
	
	deque<Task> Cloud;
	deque<Task> TBS;

	Node(){
		id = 0;
		total_U = 0.0;
		task_q.clear();
		Cloud.clear();
		TBS.clear();
		nextNode = NULL;
		preNode = NULL;
		result.clear();
	}
};

