
extern Node* NodeHead;
extern Node* GW;		// Gateway
extern Task* taskgen;	// task
extern fstream fs;

void scheduler();

/*=====================
	    Policy
=====================*/

void EDF();