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
string GENfile="..\\WBAN_GenResult\\";	//放到前一目錄下的GenResult目錄，產生txt檔
fstream fs;
int Set = 100;
int NodeNum = 3;	// # of GW Node
int TaskNum = 3;	// # of Tasks in each GW
float total_U = 1.0;	// total Utilization
float lowest_U = 0.01;	// lowest Utilization

int period[] = {100, 200, 400, 800, 1000};
int HyperPeriod = 4000;
int timeTick = 0;
//-------- Sched Policy --------------------------
#define	Nofld	0
#define	Ofld	1
#define EDF		2

int schedPolicy = EDF;	// EDF

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
/*=================================
		Main function
==================================*/
int main(){

	srand(time(0));
	
	string filename = "GW-"+to_string(NodeNum)+"_Task-"+to_string(TaskNum)+"_Set"+to_string(Set)+".txt";	// filename of Gen
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
	
	for(int set=0;set<Set;++set) {
		printf("------------- Set %03d -------------\n",set+1);
		Create();
		//printf("gen...\n");
		WBAN_Gen();
		//printf("printing...\n");
		Print_WBAN();

		GW = NodeHead;
		while(GW->nextNode != NULL) {
			GW = GW->nextNode;
			OFLD(GW);
			dispatch(GW);
		}

		printOFLD();
		printDispatch();

		scheduler(schedPolicy);

		fs << "-------------\n" << setw(3) << setfill('0') << set+1 << "\n-------------\n";

	}
	Clear();
	fs.close();
	filename.clear();
	system("PAUSE");
	return 0;
}