#include <iostream>
#include <iomanip>
#include <deque>
#include <vector>
#include <fstream>

#include "Struct.h"
#include "Sched.h"

using namespace std;

void EDF(){


}

void scheduler(int policy){
	switch (policy)
	{
	case 1:
		EDF();
		break;
	
	default:
		EDF();
		break;
	}
}