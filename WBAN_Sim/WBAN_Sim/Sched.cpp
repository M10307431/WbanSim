#include <iostream>
#include <iomanip>
#include <deque>
#include <vector>
#include <fstream>

#include "Struct.h"
#include "Sched.h"

using namespace std;


void sched_new(){



}

/*=====================
	    Policy
=====================*/
void EDF(){
	timeTick = 0;

	while(timeTick <= HyperPeriod){
	
	
	}

}

void scheduler(int policy){
	switch (policy)
	{
	case 0:
		break;

	case 1:
		break;

	case 2:
		EDF();
		break;
	
	default:
		EDF();
		break;
	}
}