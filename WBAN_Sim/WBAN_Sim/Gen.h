
extern Node* NodeHead; 	// Node head
extern Node* GW;		// Gateway
extern Task* taskgen;		// task 

/*=================================
		Parameter Setting
==================================*/
extern fstream fs;
extern int Set;
extern int NodeNum;	// # of GW Node
extern int TaskNum;	// # of Tasks in each GW
extern float total_U;	// total Utilization
extern float lowest_U;	// lowest Utilization

extern int period[];
extern int HyperPeriod;

/*=================================
		Function
==================================*/
void Create();
void WBAN_Gen();

void Print_WBAN();

void Clear(); 