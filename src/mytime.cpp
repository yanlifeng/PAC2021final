#include "mytime.h"

void printTime(timeb& start,timeb& end,string str){
    double startTime,endTime;
    startTime = start.time + start.millitm / 1000.0;
    endTime = end.time + end.millitm / 1000.0;
    printf("%s:%.3lfs\n",str.c_str(),endTime-startTime);
}