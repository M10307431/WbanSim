//#include "Struct_Gen.h"

extern Node* NodeHead;
extern Node* GW;		// Gateway
extern Task* taskgen;		// task

extern fstream fs;

extern int NodeNum;
extern int TaskNum;

extern const float speedRatio;
//--------Power--------------
extern const float p_idle;
extern const float p_comp;
extern const float p_trans;
//--------time---------------
extern const float t_trans;

//bool remote = false;	// 0:local, 1:remote

float calEnergy(bool remote, int exec, float Eng);	// remote 0:local, 1:remote

void OFLD(Node* GW);

void printOFLD();