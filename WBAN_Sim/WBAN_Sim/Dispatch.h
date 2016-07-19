
extern Node* NodeHead;
extern Node* GW;		// Gateway
extern Task* taskgen;	// task
extern fstream fs;

void q_init(Node* GW);

void setPrio(Task task);

void dispatch(Node* GW);

void printDispatch();