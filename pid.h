#include <sys/time.h>

typedef struct{
	float P,I,D,p,i,d;
	int sum_erro;
	int erro_ant;
	long mtime;
	struct timeval start;
	}PID_T;

void PID_Conf(PID_T *pid,float p,float i,float d);
float PID_Process(PID_T *pid,float erro);
