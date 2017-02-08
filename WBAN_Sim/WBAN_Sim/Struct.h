
using namespace std;

extern int HyperPeriod;
extern int timeTick;
extern const float speedRatio;
extern const int offloadTransfer;
extern const int fogTransfer;
extern const int battery;	// 2600mAh
extern float m;
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
	int uplink;
	int dwlink;
	float uti;
	int prio;

	bool offload;
	int target;
	int vm;

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
		vm = -1;
		period = 0;
		deadline = 0;
		virtualD = 0;
		exec = 0;
		uplink = 0;
		dwlink = 0;
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
	int serverEng;
	int fogEng;
	int totalTask;
	int meet;
	int miss;
	double meet_ratio;
	double lifetime;	// [CR2032] 230mA / energy (hr)
	int resp;


	void clear(){
		energy = 0;
		serverEng = 0;
		fogEng = 0;
		totalTask = 0;
		meet = 0;
		miss = 0;
		meet_ratio = 0;
		lifetime = 0;
		resp = 0;
	};

	void calculate(){
		meet_ratio = (float)meet / totalTask;
		lifetime = 5*battery*60*60 / energy;
	}
};

struct Node{
	int id;
	float total_U;

	/*=========== Migration Weight ========================*/
	int speed;				// exection speed
	float batt;			// bttery level (remaing energy)
	int block;				// blocking time
	float migration_factor;	// migration_factor
	float current_U;		// current utilization
	float migratWeight;		// migration weight
	int admin;
	int CCadm;
	int remaingFog;

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
		batt = 1.0;
		block = 0;
		migration_factor = m;		// 1.0 <<---energy------------load--->> 0.0
		migratWeight = 0.0;
		admin = 0;
		CCadm =0;
		remaingFog =0;
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
		current_U = (currTask->remaining < HyperPeriod)? (float)currTask->remaining/speed/currTask->deadline : 0;

		for(deque<Task>::iterator it=local_q.ready_q.begin(); it!=local_q.ready_q.end(); ++it){
			current_U += (float)it->uti/speed;
		}
		for(deque<Task>::iterator it=local_q.wait_q.begin(); it!=local_q.wait_q.end(); ++it){
			current_U += (float)it->uti/speed;
		}
		for(deque<Task>::iterator it=remote_q.ready_q.begin(); it!=remote_q.ready_q.end(); ++it){
			if(it->parent != id){
				current_U += (float)it->remaining/speed/it->deadline;
			}
			else {
				if(it->target != -1){
					current_U += (float)it->uti/speed;
				}
				else{
					current_U +=  2*((float)it->remaining/it->deadline);
				}
			}
		}
		for(deque<Task>::iterator it=remote_q.wait_q.begin(); it!=remote_q.wait_q.end(); ++it){
			
			current_U += (it->target != -1)? (float)it->uti/speed : 2*(float)it->uplink/(it->period-offloadTransfer-it->exec/speedRatio); //2*(float)it->uplink/(it->period-fogTransfer-it->exec)
		}
	}

	void MW(int Eng, float utiSum, int pt){

		float Q;
		Q = (batt*(float)battery-result.energy-Eng)/(float)battery;
		/*
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
		for(deque<Task>::iterator it=local_q.wait_q.begin(); it!=local_q.wait_q.end(); ++it){
			if(it->deadline <= pt && it->deadline >= timeTick){
				block += it->remaining;
			}
		}
		for(deque<Task>::iterator it=remote_q.ready_q.begin(); it!=remote_q.ready_q.end(); ++it){
			if(it->deadline <= pt && it->deadline >= timeTick){
				block += it->remaining;
			}
		}
		for(deque<Task>::iterator it=remote_q.wait_q.begin(); it!=remote_q.wait_q.end(); ++it){
			if(it->deadline <= pt && it->deadline >= timeTick){
				block += it->remaining;
			}
		}
		*/
		migratWeight = migration_factor*Q - (1.0-migration_factor)*(current_U);
	}

	void ADM(int exec, int deadline2, int uplink){

		int exec_pr = (currTask->deadline <= deadline2)? currTask->remaining : 0;
		
		for(deque<Task>::iterator it=local_q.ready_q.begin(); it!=local_q.ready_q.end(); ++it){
			if(it->deadline < deadline2 && it->deadline >= timeTick){
				exec_pr += it->remaining/speed;
			}
		}
		for(deque<Task>::iterator it=remote_q.ready_q.begin(); it!=remote_q.ready_q.end(); ++it){
			if(it->deadline < deadline2 && it->deadline >= timeTick){
				exec_pr += it->remaining/speed;
			}/*
			if(it->deadline < deadline2 && it->deadline >= timeTick && it->parent != id){
				exec_pr += it->remaining/speed;
			}
			else if(it->deadline <= deadline2 && it->deadline >= timeTick){
				exec_pr += (it->target != -1)? it->exec/speed : it->remaining;
			}*/
		}
		for(deque<Task>::iterator it=local_q.wait_q.begin(); it!=local_q.wait_q.end(); ++it){
			if(it->deadline+it->period <= deadline2){
				exec_pr += it->exec/speed;
			}
		}
		for(deque<Task>::iterator it=remote_q.wait_q.begin(); it!=remote_q.wait_q.end(); ++it){
			exec_pr += it->uplink;
			/*if((it->target != -1) && (it->deadline+it->period <= deadline2)){
				exec_pr += it->exec/speed;
			}
			else if((it->target == -1) && (it->deadline+it->period-offloadTransfer-it->exec/speedRatio <= deadline2)){
				exec_pr += it->uplink;
			}*/
		}
		
		admin = exec_pr + exec/speed + remaingFog;
		admin = (admin > deadline2-timeTick-uplink)? -1 : admin;
	}

	void cloudADM(int exec, int deadline2, int uplink){
		int exec_pr = 0;

		for(deque<Task>::iterator it=Cloud.begin(); it!=Cloud.end(); ++it){
			if(it->deadline <= deadline2 ){
				exec_pr += it->remaining/speedRatio;
			}
		}

		CCadm = exec_pr + exec/speedRatio;
		CCadm = (CCadm > deadline2-timeTick-uplink)? -1 : CCadm; 
	}
};

