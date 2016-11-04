#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <ctime>
#include <deque>
#include <vector>
#include <algorithm>

#include "Struct.h"
#include "Gen.h"
#include "OFLDecision.h"
#include "Dispatch.h"
#include "Sched.h"
#include "Debug.h"

using namespace std;

#define NOFLD 0
#define myOFLD 1
#define AOFLDC 2
#define AOFLDF 3
int policyOFLD = NOFLD;	// 0:nver, 1:my, 2:always

/*=================================
		  System componet
==================================*/
Node* NodeHead = new Node;	// Node head
Node* GW = new Node;		// Gateway
Task* taskgen = new Task;	// task
Task* idleTask = new Task;	// idle task


/*=================================
		  Setting
==================================*/
bool inputLoad = 1;	// 0: Gen, 1: inputfile 
string GENfile="..\\WBAN_GenResult\\";	//放到前一目錄下的GenResult目錄，產生txt檔
char* configPath = "config.txt";
char* inputPath = "input.txt";
fstream fs, config, input;
int Set = 100;
int NodeNum = 3;	// # of GW Node
int TaskNum = 4;	// # of Tasks in each GW
float total_U = 1.5;	// total Utilization
float lowest_U = 0.05;	// lowest Utilization

int period[] = {100, 200, 400, 800, 1000};
int HyperPeriod = 4000;
int timeTick = 0;
//-------- Sched Policy --------------------------
#define	Nofld	0
#define	Ofld	1
#define EDF		2

int schedPolicy = EDF;	// EDF
int fogspeed = 2;
/*=================================
          Parameter
=================================*/

const float speedRatio = 10;	// remoteSpeed / localSpeed
//--------Power-------------------------------------------------------
const float p_idle = 1.55;	// idle (W)
const float p_comp	= 2.9-1.55;	// full load
const float p_trans = 0.3;	// wifi trans	
//--------Time--------------------------------------------------------
const float t_trans = 25; // wifi trans time (ms)

const int offloadTransfer = 25;
const int fogTransfer = 5;
/*=================================
		Main function
==================================*/
int main(){

	srand(time(0));
	
	if(inputLoad) {
	//-------------------------------------------------------------------------- Config Setting
		char strBuf[20];
		char* value;
		config.clear();
		config.open(configPath, std::fstream::in);
		if(!config){
			cout << "Fail to open file: " << configPath << endl;
			system("PAUSE");
			return 0;
		}
		else{
			config.getline(strBuf, sizeof(strBuf));
			value = strtok(strBuf, " = ");
			value = strtok(NULL, " = ");
			Set = atoi(value);

			config.getline(strBuf, sizeof(strBuf));
			value = strtok(strBuf, " = ");
			value = strtok(NULL, " = ");
			NodeNum = atoi(value);

			config.getline(strBuf, sizeof(strBuf));
			value = strtok(strBuf, " = ");
			value = strtok(NULL, " = ");
			TaskNum = atoi(value);

			config.getline(strBuf, sizeof(strBuf));
			value = strtok(strBuf, " = ");
			value = strtok(NULL, " = ");
			total_U = atof(value);

			config.getline(strBuf, sizeof(strBuf));
			value = strtok(strBuf, " = ");
			value = strtok(NULL, " = ");
			if(strcmp(value, "myOFLD") == 0){policyOFLD = myOFLD;}
			else if(strcmp(value, "AOFLDC") == 0){policyOFLD = AOFLDC;}
			else if(strcmp(value, "AOFLDF") == 0){policyOFLD = AOFLDF;}
			else if(strcmp(value, "NOFLD") == 0){policyOFLD = NOFLD;}

			config.getline(strBuf, sizeof(strBuf));
			value = strtok(strBuf, " = ");
			value = strtok(NULL, " = ");
			if(strcmp(value, "EDF") == 0){schedPolicy = EDF;}
		
			config.close();
		}

	//--------------------------------------------------------------------------- Load Task Set
		string filename = "input_GW-"+to_string(NodeNum)+"_Task-"+to_string(TaskNum)+"_U"+to_string(total_U)+".txt";	// filename of Gen
		char *inputPath=(char*)filename.c_str();
		input.clear();
		input.open(inputPath, std::fstream::in);
		if(!input){
			cout << "Fail to open file: " << inputPath << endl;
			system("PAUSE");
			return 0;
		}
	}
	else{
	//--------------------------------------------------------------------------- Gen Task Set
		string filename = "input_GW-"+to_string(NodeNum)+"_Task-"+to_string(TaskNum)+"_U"+to_string(total_U)+".txt";	// filename of Gen
		char *GENbuffer=(char*)filename.c_str();
		input.clear();
		input.open(GENbuffer, std::fstream::out); //  in:read / out:write / app:append
		if(!input) {	//如果開啟檔案失敗
				cout << "Fail to open file: " << GENbuffer << endl;
				filename.clear();
				system("PAUSE");
				return 0;
		}
	}


	//--------------------------------------------------------------------------- Store Result
	string filename = "Result_GW-"+to_string(NodeNum)+"_Task-"+to_string(TaskNum)+"_U"+to_string(total_U)+"_"+to_string(policyOFLD)+".txt";	// filename of Gen
	string GENBuffer = GENfile + filename;	//Gen 路徑+檔名
	char *GENbuffer=(char*)GENBuffer.c_str();
	fs.clear();
	fs.open(GENbuffer, std::fstream::out); //  in:read / out:write / app:append
	if(!fs) {	//如果開啟檔案失敗
			cout << "Fail to open file: " << GENbuffer << endl;
			filename.clear();
			system("PAUSE");
			return 0;
	}

	Result* Result_avg = new Result;
	Result_avg->clear();

	for(int set=0;set<Set;++set) {
		printf("------------- Set %03d -------------\n",set+1);
		Create();

		if(inputLoad){
			WBAN_Load();
			Print_WBAN();
		}
		else{
			WBAN_Gen();
			Output_WBAN();
			input << "-------------\n" << setw(3) << setfill('0') << set+1 << "\n-------------\n";
		}
		

		//policyOFLD = myOFLD;
		GW = NodeHead;
		while(GW->nextNode != NULL) {
			GW = GW->nextNode;
			OFLD(GW);
			dispatch(GW);
			if(GW->nextNode == NULL){	//fog
				GW->speed = fogspeed;
			}
		}

		printOFLD();
		printDispatch();

		scheduler(schedPolicy);

		// calculate meet_ratio & lifetime
		GW = NodeHead;
		while(GW->nextNode != NULL) {
			GW = GW->nextNode;
			GW->result.calculate();
			printResult(GW);
			if(GW->nextNode != NULL){
				Result_avg->energy += GW->result.energy;
				Result_avg->meet_ratio += GW->result.meet_ratio;
			}
		}

		//////////////////////////////////////////
		//fs << "--------- Never OFLD ----------\n";
		//policyOFLD = NOFLD;
		//GW = NodeHead;
		//while(GW->nextNode != NULL) {
		//	GW = GW->nextNode;
		//	GW->Cloud.clear();
		//	GW->TBS.clear();
		//	GW->local_q.ready_q.clear();
		//	GW->local_q.wait_q.clear();
		//	GW->remote_q.ready_q.clear();
		//	GW->remote_q.wait_q.clear();
		//	GW->result.clear();
		//	GW->currTask = idleTask;
		//	OFLD(GW);
		//	dispatch(GW);
		//}

		//printOFLD();
		//printDispatch();

		//scheduler(schedPolicy);

		//// calculate meet_ratio & lifetime
		//GW = NodeHead;
		//while(GW->nextNode != NULL) {
		//	GW = GW->nextNode;
		//	GW->result.calculate();
		//	printResult(GW);
		//}

		//////////////////////////////////////////
		//fs << "--------- Always OFLD ----------\n";
		//policyOFLD = AOFLD;
		//GW = NodeHead;
		//while(GW->nextNode != NULL) {
		//	GW = GW->nextNode;
		//	GW->Cloud.clear();
		//	GW->TBS.clear();
		//	GW->local_q.ready_q.clear();
		//	GW->local_q.wait_q.clear();
		//	GW->remote_q.ready_q.clear();
		//	GW->remote_q.wait_q.clear();
		//	GW->result.clear();
		//	GW->currTask = idleTask;
		//	OFLD(GW);
		//	dispatch(GW);
		//}

		//printOFLD();
		//printDispatch();

		//scheduler(schedPolicy);

		//// calculate meet_ratio & lifetime
		//GW = NodeHead;
		//while(GW->nextNode != NULL) {
		//	GW = GW->nextNode;
		//	GW->result.calculate();
		//	printResult(GW);
		//}
		fs << "-------------\n" << setw(3) << setfill('0') << set+1 << "\n-------------\n";

	}
	fs << "============== Average Result ==============\n";
	fs << "Meet Ratio : " << Result_avg->meet_ratio/(Set*2) << endl;
	fs << "Energy Consumption : " << Result_avg->energy/(Set*2) << endl;

	Clear();
	fs.close();
	input.close();
	filename.clear();
	system("PAUSE");
	return 0;
}