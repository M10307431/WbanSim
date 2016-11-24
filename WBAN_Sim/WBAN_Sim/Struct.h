
using namespace std;

extern int timeTick;
extern const int offloadTransfer;
extern const int fogTransfer;

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
	int resp;


	void clear(){
		energy = 0;
		totalTask = 0;
		meet = 0;
		miss = 0;
		meet_ratio = 0;
		lifetime = 0;
		resp = 0;
	};

	void calculate(){
		meet_ratio = (float)meet / totalTask;
		lifetime = 0.23*60*60 / energy;
	}
};

struct Node{
	int id;
	float total_U;

	/*=========== Migration Weight ========================*/
	int speed;				// exection speed
	int batt;			// bttery level (remaing energy)
	int block;				// blocking time
	float migration_factor;	// migration_factor
	float current_U;		// current utilization
	float migratWeight;		// migration weight

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
		speed = 1;
		batt = 100;
		block = 0;
		migration_factor = 0.0;		// 1.0 <<---energy------------load--->> 0.0
		migratWeight = 0.0;
		total_U = 0.0;
		current_U = 0.0;
		task_q.clear();
		Cloud.clear();
		TBS.clear();
		nextNode = NULL;
		preNode = NULL;
		currTask = NULL;
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
				current_U += (float)(it->exec-2*fogTransfer)/it->period;
				//current_U += (float)(it->remaining-fogTransfer)/(speed*(it->deadline-timeTick));
			}
			else {
				current_U +=  (float)offloadTransfer*2/it->period;
			}
		}
		for(deque<Task>::iterator it=remote_q.wait_q.begin(); it!=remote_q.wait_q.end(); ++it){
			if(it->parent != id){
				current_U += (float)(it->exec-2*fogTransfer)/it->period;
			}
			else {
				current_U +=  (float)offloadTransfer*2/it->period;
			}
		}
	}

	void MW(int battSum, float utiSum, int pt){
		
		if(currTask->id != 999 && currTask->deadline < pt){			// idle task
			block = currTask->remaining;
		}
		else{
			block = 0;
		}

		for(deque<Task>::iterator it=local_q.ready_q.begin(); it!=local_q.ready_q.end(); ++it){
			if(it->deadline <= pt && it->deadline >= timeTick){
				block += it->remaining;
			}
		}
		/*for(deque<Task>::iterator it=local_q.wait_q.begin(); it!=local_q.wait_q.end(); ++it){
			if(it->deadline <= pt && it->deadline >= timeTick){
				block += it->remaining;
			}
		}*/
		for(deque<Task>::iterator it=remote_q.ready_q.begin(); it!=remote_q.ready_q.end(); ++it){
			if(it->deadline <= pt && it->deadline >= timeTick){
				block += it->remaining;
			}
		}
		/*for(deque<Task>::iterator it=remote_q.wait_q.begin(); it!=remote_q.wait_q.end(); ++it){
			if(it->deadline <= pt && it->deadline >= timeTick){
				block += it->remaining;
			}
		}*/

		if((pt-timeTick) != 0){
			migratWeight = migration_factor*((float)batt/battSum)-(1.0-migration_factor)*(current_U/utiSum)-((float)block/(pt-timeTick));
		}
		else{
			migratWeight = -999;
		}
	}
};

