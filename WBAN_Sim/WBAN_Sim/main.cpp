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
#define SeGW 4
int policyOFLD = NOFLD;	// 0:nver, 1:my, 2:always

int sameP=0;
vector<int> missSet;

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
int TaskNum = 20;	// # of Tasks in each GW
float total_U = 2.0;	// total Utilization
float lowest_U = 0.05;	// lowest Utilization
float m = 0.5;			// migration factor	 1.0 <<---energy------------load--->> 0.0

int period[] = {100, 200, 400, 800, 1000};
int HyperPeriod = 4000;
int timeTick = 0;
//-------- Sched Policy --------------------------
#define	FIFO	1
#define EDF		2

int schedPolicy = EDF;	// EDF
int fogspeed = 1;
int fogserver =0;		// fog server num off/on
/*=================================
          Parameter
=================================*/
const int battery = 5*1000*3600/1000;	// 5v * 2600mA *3600s	//700mAh
const float speedRatio = 5;	// remoteSpeed / localSpeed
const int WBANpayload = 128; // WBAN payload for normalized (byte)
//--------Power-------------------------------------------------------
const float p_idle = 1.8;		// idle (W)	1.55
const float p_comp	= 4.4-1.8;	// full load	29.-1.55
const float p_trans = 0.5;		// wifi trans	0.3
const int cloudp_idle = 223;	// server idle power (W)
const int cloudp_actv = 368;	// server active power
//--------Time--------------------------------------------------------
const float _traffic = 1/0.5;			// 1/bandwidth(0-1.0)  >> 1, 1/0.75, 1/0.5
const int proc = 5;
const int offloadTransfer = 25;	// global trans time (ms)
const int fogTransfer = 5;		// local trans
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
			else if(strcmp(value, "SeGW") == 0){policyOFLD = SeGW;}

			config.getline(strBuf, sizeof(strBuf));
			value = strtok(strBuf, " = ");
			value = strtok(NULL, " = ");
			if(strcmp(value, "EDF") == 0){schedPolicy = EDF;}
			if(strcmp(value, "FIFO") == 0){schedPolicy = FIFO;}
		
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
	vector<double> GW_Eng(NodeNum, 0);

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
			GW->result.clear();
			if(GW->nextNode != NULL)
				OFLD(GW);
			dispatch(GW);
			if(GW->nextNode == NULL){	//fog
				GW->speed = fogspeed;
			}
		}

		printOFLD();
		printDispatch();

		if(inputLoad){
			//HyperPeriod = NodeHead->nextNode->task_q.front().period;	// !!!!!!!!!!!!!!!!!just for resp test
			scheduler(schedPolicy);
		}

		// calculate meet_ratio & lifetime
		GW = NodeHead;
		while(GW->nextNode != NULL) {
			GW = GW->nextNode;
			GW->result.calculate();
			printResult(GW);
			GW_Eng[GW->id] += GW->result.energy;	// each GW energy
			if(GW->nextNode != NULL){
				Result_avg->energy += GW->result.energy;
				Result_avg->meet_ratio += GW->result.meet_ratio;
				//Result_avg->resp += GW->result.resp; // just for meet task
			}
			else{
				Result_avg->serverEng += GW->result.serverEng;
				Result_avg->fogEng += GW->result.energy;
			}
		}

		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		if(NodeHead->nextNode->result.meet_ratio<1 || NodeHead->nextNode->nextNode->result.meet_ratio<1){
			missSet.push_back(set+1);
		}
		Result_avg->resp += NodeHead->nextNode->result.resp + NodeHead->nextNode->nextNode->result.resp;
		sameP += 2*HyperPeriod;

		fs << "-------------\n" << setw(3) << setfill('0') << set+1 << "\n-------------\n";

	}
	fs << "============== Average Result ==============\n";
	fs << "Meet Ratio : " << Result_avg->meet_ratio/(Set*2) << endl;
	fs << "Energy Consumption : " << Result_avg->energy/(Set*2) << endl;
	fs << "Response time of Meet : " << Result_avg->resp << endl;
	fs << "Cloud server energy : " << Result_avg->serverEng/Set << endl;
	fs << "Fog devices energy : " << Result_avg->fogEng/Set << endl << endl;
	
	GW = NodeHead;
	while(GW->nextNode != NULL) {
		GW = GW->nextNode;
		fs << "GW_Eng : " << GW_Eng[GW->id]/Set << endl;
	}
	fs << endl;

	fs << "HyerP : " << sameP << "\nMiss set : ";
	for(int i=0;i<missSet.size();++i){
		fs << missSet[i] << " ";
	}
	fs << "size: " << Set-missSet.size();

	Clear();
	fs.close();
	input.close();
	filename.clear();
	//system("PAUSE");
	return 0;
}