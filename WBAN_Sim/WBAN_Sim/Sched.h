extern Node* NodeHead;
extern Node* GW;		// Gateway
extern Task* taskgen;	// task
extern Task* idleTask;
extern fstream fs;

extern int HyperPeriod;
extern int timeTick;

extern const int offloadTransfer;
extern const float speedRatio;	// remoteSpeed / localSpeed

void scheduler(int policy);

/*=====================
	    Policy
=====================*/

void EDF();

void printSched();