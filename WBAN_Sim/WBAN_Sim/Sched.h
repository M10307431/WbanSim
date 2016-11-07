extern Node* NodeHead;
extern Node* GW;		// Gateway
extern Task* taskgen;	// task
extern Task* idleTask;
extern fstream fs;

extern int HyperPeriod;
extern int timeTick;

extern const int offloadTransfer;
extern const int fogTransfer;
extern const float speedRatio;	// remoteSpeed / localSpeed

extern int policyOFLD;

void scheduler(int policy);

//--------Power-------------------------------------------------------
extern const float p_idle;	// idle (W)
extern const float p_comp;	// full load
extern const float p_trans;	// wifi trans	
//--------Time--------------------------------------------------------
extern const float t_trans; // wifi trans time (ms)

/*=====================
	    Policy
=====================*/
void FIFO();
void EDF();

void sched_new(Node* GW);
void sched_fifo(Node* GW);
void cludServer(Node* GW);
void migration(Node* src, Node* dest);
void EvaluationFog(Task *task);
Node *findMigraDest(Task *task);
Node *backMigraSrc(Task *task);

void printSched(char* state);
void printResult(Node* GW);