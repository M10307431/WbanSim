
extern Node* NodeHead; 	// Node head
extern Node* GW;		// Gateway
extern Task* taskgen;		// task 
extern Task* idleTask;

/*=================================
		Parameter Setting
==================================*/
extern fstream fs, input;
extern char *inputPath;
extern int Set;
extern int NodeNum;	// # of GW Node
extern int TaskNum;	// # of Tasks in each GW
extern float total_U;	// total Utilization
extern float lowest_U;	// lowest Utilization

extern bool GW431;

extern int period[];
extern int HyperPeriod;

extern int fogserver;
/*=================================
		Function
==================================*/
void Create();
void WBAN_Gen();
void WBAN_Load();

void Output_WBAN();
void Print_WBAN();

void Clear(); 