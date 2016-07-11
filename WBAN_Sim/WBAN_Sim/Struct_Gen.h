using namespace std;

/*=================================
		Structure
==================================*/
struct Task{
	int id;
	int period;
	int exec;
	float uti;

	bool offload;
	float localEng;
	float remoteEng;
	
	Task(){
		id = 0;
		period = 0;
		exec = 0;
		uti = 0;
		offload = false;
		localEng = 0;
		remoteEng = 0;
	}
};

struct Node{
	int id;
	float total_U;
	deque<Task> task_q;

	struct Node* nextNode;
	struct Node* preNode;

	Node(){
		id = 0;
		total_U = 0.0;
		task_q.clear();
		nextNode = NULL;
		preNode = NULL;
	}
};

/*=================================
		Function
==================================*/
void Create();
void WBAN_Gen();

void Print_WBAN();

void Clear(); 