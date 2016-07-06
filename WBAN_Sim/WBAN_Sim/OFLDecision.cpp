//#include <iostream>
//#include <iomanip>
//#include <fstream>
//#include <string>
//#include <ctime>
//#include <deque>
//#include <vector>
//#include <algorithm>
//#include <fstream>
//
//#include "WBAN_Gen.h"
//
//using namespace std;
//
//Node* NodeHead = new Node;	// Node head
//Node* GW = new Node;		// Gateway
//Task* taskgen = new Task;		// task 
//
///*=================================
//		Parameter Setting
//==================================*/
//string GENfile="..\\WBAN_GenResult\\";	//放到前一目錄下的GenResult目錄，產生txt檔
//fstream fs;
//int Set = 100;
//int NodeNum = 3;	// # of GW Node
//int TaskNum = 3;	// # of Tasks in each GW
//float total_U = 1.0;	// total Utilization
//float lowest_U = 0.01;	// lowest Utilization
//
//int period[] = {100, 200, 400, 800, 1000};
//int HyperPeriod = 4000;
//
///*=================================
//		Function
//==================================*/
//void Create();
//void WBAN_Gen();
//
//void Print_WBAN();
//
//void Clear(); 
//
///*======== subfunction ============*/
//bool cmp(float i, float j) {return i > j;}
//
///*=================================
//		Construct Node & Task
//==================================*/
//void Create(){
//	Clear();
//	NodeHead = new Node;
//	NodeHead->nextNode = NULL;
//	NodeHead->preNode = NodeHead;
//
//	/*=========================== Gen GW ============================*/
//	for(int n =0; n<NodeNum; ++n){
//		GW = new Node;
//		GW->preNode = NodeHead->preNode;
//		GW->nextNode = NULL;
//		GW->id = n;
//		NodeHead->preNode->nextNode = GW;
//		NodeHead->preNode = GW;
//
//		/*---------------- Gen task -----------------*/
//		for(int t=0; t<TaskNum; ++t){
//			taskgen = new Task;
//			GW->task_q.push_back(*taskgen);
//		}
//	}
//}
//
///*=================================
//		Generate Node & Task
//==================================*/
//void WBAN_Gen(){
//	GW = NodeHead;
//	while (GW->nextNode != NULL){
//		GW = GW->nextNode;
//		float remain_U = total_U;
//		vector<float> U;
//		vector<int> P;
//		for(int t=0; t<TaskNum; ++t){
//			float uti = (t==TaskNum-1) ? remain_U : rand()/RAND_MAX;						// the last task uti = remain uti
//			while(uti<lowest_U || uti>remain_U || uti>remain_U-(TaskNum-t-1)*lowest_U)		// uti > 0.01
//				uti = (float)rand()/RAND_MAX;
//			
//			P.push_back(period[rand()% sizeof(*period)]);
//			U.push_back(uti);
//			remain_U -= uti;
//		}
//		// shorter period has larger utilization
//		sort(P.begin(),P.end());
//		sort(U.begin(),U.end(),cmp);
//		// set task parameters to node
//		for(int t=0; t<TaskNum; ++t){
//			GW->task_q.at(t).id = t;										// task id
//			GW->task_q.at(t).period = P.at(t);								// task period
//			GW->task_q.at(t).exec = U.at(t) * P.at(t);						// task execution
//			GW->task_q.at(t).uti = (float)GW->task_q.at(t).exec/P.at(t);	// task utilization
//			GW->total_U += GW->task_q.at(t).uti;							// node total utilization
//		}
//		
//	}
//}
//
///*=================================
//		Clear all nodes
//==================================*/
//void Clear(){
//	GW = NodeHead;
//	while(GW->nextNode!=NULL){
//		GW->task_q.~deque();
//		GW = GW->nextNode;
//		delete GW->preNode;	// first time would delete NodeHead
//	}
//	GW->task_q.~deque();
//	delete GW;
//}
///*=================================
//		Print Gen Result
//==================================*/
//void Print_WBAN(){
//	GW = NodeHead->nextNode;
//	fs << NodeNum << " " << TaskNum << endl;
//	while (GW != NULL){
//		cout << "Gateway Node : "<< GW->id << "\tU = " << GW->total_U << endl;
//		fs << "GW " << GW->total_U << endl;
//		for(deque<Task>::iterator it=GW->task_q.begin(); it!=GW->task_q.end(); ++it){
//			cout << "\t T" << it->id << " = (" << it->exec << ", " << it->period << ", " << it->uti << ")\n";
//			fs << "Task "<< it->exec << " " << it->period << " " << it->uti << "\n";
//		}
//		cout<<endl;
//		GW = GW->nextNode;
//	}
//	cout<<endl;
//}
//
///*=================================
//		Main function
//==================================*/
//int main(){
//
//	srand(time(0));
//	
//	string filename = "GW-"+to_string(NodeNum)+"_Task-"+to_string(TaskNum)+"_Set"+to_string(Set)+".txt";	// filename of Gen
//	string GENBuffer = GENfile + filename;	//Gen 路徑+檔名
//	char *GENbuffer=(char*)GENBuffer.c_str();
//	fs.clear();
//	fs.open(GENbuffer, std::fstream::out); //  in:read / out:write / app:append
//	if(!fs) {	//如果開啟檔案失敗
//			cout << "Fail to open file: " << GENbuffer << endl;
//			filename.clear();
//			system("PAUSE");
//			return 0;
//	}
//	
//	for(int set=0;set<Set;++set){
//		printf("------------- Set %03d -------------\n",set+1);
//		Create();
//		//printf("gen...\n");
//		WBAN_Gen();
//		//printf("printing...\n");
//		Print_WBAN();
//		fs << "-------------\n" << setw(3) << setfill('0') << set+1 << "\n-------------\n";
//	}
//	Clear();
//	fs.close();
//	filename.clear();
//	system("PAUSE");
//	return 0;
//}