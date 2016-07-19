
extern Node* NodeHead;
extern Node* GW;		// Gateway
extern Task* taskgen;	// task
extern fstream fs;

extern int HyperPeriod;
extern int timeTick;

void scheduler();

/*=====================
	    Policy
=====================*/

void EDF();