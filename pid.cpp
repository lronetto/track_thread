#include <iostream>
#include <stdio.h>

#include <sys/time.h>
#include "pid.h"

void PID_Conf(PID_T *pid,float p,float i,float d){
	struct timeval start;
	pid->P=p;
	pid->I=i;
	pid->D=d;
	pid->sum_erro=0;
	pid->erro_ant=0;
	gettimeofday(&start, 0);
	pid->start=start;
	}
float PID_Process(PID_T *pid,float erro){
	long mtime, seconds, useconds;
	float ret;
	struct timeval end, start;
	start=pid->start;
	gettimeofday(&end, 0);
	seconds  = end.tv_sec  - start.tv_sec;
   	useconds = end.tv_usec - start.tv_usec;
	mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
	pid->mtime=mtime;
	pid->sum_erro+=erro;
	float p=pid->P*erro;
	float i=pid->I*(pid->sum_erro)*mtime;
	float d=pid->D*(pid->erro_ant-erro)/mtime;
	pid->p=p;
	pid->i=i;
	pid->d=d;
	ret= p + i - d;
	//printf("mtime=%d p=%2.2f i=%2.2f d=%2.2f ret=%2.2f \n",mtime,p,i,d,ret);
	pid->erro_ant=erro;
	gettimeofday(&start, 0);
	pid->start=start;	
	return ret;
	}
