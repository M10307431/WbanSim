
using namespace std;

extern const int offloadTransfer;

/*=================================
		Structure
==================================*/
struct Task{
	int id;
	int cnt;
	int parent;
	
	int period;
	int deadline;
	int virtualD;
	int exec;
	int remaining;
	float uti;
	int prio;

	bool offload;
	int target;

	float localEng;
	float remoteEng;
	
	void _setPrio(int _prio){
		prio = _prio;
	};

	Task(){
		id = -1;
		cnt = -1;
		parent =-1;
		target = -1;
		period = 0;
		deadline = 0;
		virtualD = 0;
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
	float current_U;
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
		current_U = 0.0;
		task_q.clear();
		Cloud.clear();
		TBS.clear();
		nextNode = NULL;
		preNode = NULL;
		result.clear();
	}

	void update_U(){
		current_U = currTask->uti;

		for(deque<Task>::iterator it=local_q.ready_q.begin(); it!=local_q.ready_q.end(); ++it){
			current_U += it->uti;
		}
		for(deque<Task>::iterator it=local_q.wait_q.begin(); it!=local_q.wait_q.end(); ++it){
			current_U += it->uti;
		}
		for(deque<Task>::iterator it=remote_q.ready_q.begin(); it!=remote_q.ready_q.end(); ++it){
			if(it->parent != id){
				current_U += (float)(it->exec)/it->period;
			}
			else {
				current_U +=  (float)offloadTransfer*2/it->period;
			}
		}
		for(deque<Task>::iterator it=remote_q.wait_q.begin(); it!=remote_q.wait_q.end(); ++it){
			if(it->parent != id){
				current_U += (float)(it->exec)/it->period;
			}
			else {
				current_U +=  (float)offloadTransfer*2/it->period;
			}
		}
	}
};

