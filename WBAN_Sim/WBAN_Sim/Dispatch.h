
extern Node* NodeHead;
extern Node* GW;		// Gateway
extern Task* taskgen;	// task
extern fstream fs;

void q_init(Node* GW);

int setPrio(deque<Task>::iterator task);

void dispatch(Node* GW);

void printDispatch();