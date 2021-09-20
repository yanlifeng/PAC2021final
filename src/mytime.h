#include<ctime>
#include <sys/timeb.h>
#include <iostream>
using namespace std;
#define PTIME(str) printf("%s: %.3lf\n",str,(double)(clock()-t0)/CLOCKS_PER_SEC);

void printTime(timeb& start,timeb& end,string str);