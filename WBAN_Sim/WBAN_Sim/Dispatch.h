
deque<Task> local_q;	// local queue
deque<Task> remote_q;	// remote queue

extern Node* NodeHead;
extern Node* GW;		// Gateway
extern Task* taskgen;	// task
extern fstream fs;

void dispatch();