#include <iostream>
#include <iomanip>
#include <deque>
#include <vector>
#include <fstream>

#include "Struct.h"
#include "Dispatch.h"
#include "Debug.h"

using namespace std;

#define DEBUG 1
#define NOFLD 0
#define myOFLD 1
#define AOFLDC 2
#define AOFLDF 3
#define SeGW 4

#ifdef DEBUG
#define debug(x) printf x
#endif

void q_init(Node* GW){
	
	GW->local_q.ready_q.clear();
	GW->local_q.wait_q.clear();

	GW->remote_q.ready_q.clear();
	GW->remote_q.wait_q.clear();

	GW->total_U = 0;
}
/****************************************
            Priority Setting (�����糣�Odeadline driven�A�w�L�ϥ�)
****************************************/
int setPrio(deque<Task>::iterator task){
	switch (task->offload) {
		case true:
			return 0;

		case false:
			return 1;
		
		default:
			return 99;
	}
}

/****************************************
               Dispacher
****************************************/
void dispatch(Node* GW){
	
	q_init(GW);

	/*
		���ttask�������Q��
		�]�wparent�ήɶ��Ѽ�
	*/
	for(deque<Task>::iterator it=GW->task_q.begin(); it!=GW->task_q.end(); ++it){
		if(it->offload == true){
			it->_setPrio(setPrio(it));
			it->parent = GW->id;			// ����parent node (source)
			it->virtualD = it->deadline;	// ���������deadline�A�bsched�Τ@���O��deadline�A�ҥHVDeadline�O�ΨӰO����deadline�A�H�K�b�ĤG���q�^�_��deadline�ϥ�
			it->uplink = (it->target == -1)? offloadTransfer : fogTransfer;
			it->dwlink = (it->target == -1)? offloadTransfer : fogTransfer;
			it->remaining = it->uplink;
			if(policyOFLD==myOFLD) {
				//it->deadline = (it->target != -1)? (it->period-fogTransfer-it->exec) : (it->period-offloadTransfer-it->exec/speedRatio); // �p��virtual deadline
				it->deadline = (it->target != -1)? (it->period-fogTransfer) : (it->period-offloadTransfer); // �p��virtual deadline
			}
			if(policyOFLD==AOFLDC){
					it->deadline--;
			}

			GW->remote_q.ready_q.push_back(*it);	// �]�w�n��task��iQ��
			GW->result.totalTask++; 
		}
		else{
			it->_setPrio(setPrio(it));
			it->parent = GW->id;
			GW->local_q.ready_q.push_back(*it);
			GW->result.totalTask++;
		}
		GW->currTask = idleTask;
	}
}

void printDispatch(){
	
	GW = NodeHead->nextNode;
	fs << "---------- Dispach ------------" << endl;
	while (GW != NULL){
		
		fs << "GW" << GW->id << endl;
		fs << "Local" << endl;

		for(deque<Task>::iterator it=GW->local_q.ready_q.begin(); it!=GW->local_q.ready_q.end(); ++it){

			fs << it->id << " " ;
		}
		fs << endl << "Remote" << endl;
		
		for(deque<Task>::iterator it=GW->remote_q.ready_q.begin(); it!=GW->remote_q.ready_q.end(); ++it){

			fs << it->id << " " ;
		}
		fs << endl;
		GW = GW->nextNode;
	}
	fs << endl;
}