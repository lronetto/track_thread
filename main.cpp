#include <termios.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <curses.h>
#include <iostream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <unistd.h>
#include <pigpio.h>
#include "pid.h"
#include "servo.h"
fsadfasdf
using namespace cv;
using namespace std;
#define LED1	2
#define	LED2	5

int gpio[4]={servo_pin_x,servo_pin_y,LED1,LED2};

typedef struct{
	int x,y;
	float cont_x,cont_y,ret_x,ret_y,erro_x_t,erro_y_t;
	int flag_log;
	PID_T pid_x,pid_y;
	}thread_args;

int flag_stop=0;
int flag_servo=0;
int flag_led=0;
int flag_log=0;
int flag_pid=0;

float cont_x=1500,cont_y=1500;
pthread_cond_t cond,cond1;
pthread_mutex_t mutex,mutex1;

void isr(int gpio, int level, uint32_t tick) {
  printf("isr - gpio: %d, level: %d\n", gpio, level);
}

int kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if(ch != EOF)
	  return ch;

  return 0;
}



void *thread_log(void *vargp){
	PID_T pid_x,pid_y;
	int x,y;
	float cont_x,cont_y,ret_x,ret_y,erro_x_t,erro_y_t;
	thread_args *a = (thread_args *) vargp;
	FILE *fp;
	fp=fopen("./arq.csv","w+");
	//fclose(fp);
	while(1){
		usleep(100);
		if(flag_stop==3){
			break;
		}

		if(flag_log){
			pid_x=a->pid_x;
			pid_y=a->pid_y;

			cont_x=a->cont_x;
			cont_y=a->cont_y;
			ret_x=a->ret_y;
			erro_x_t=a->erro_x_t;
			erro_y_t=a->erro_y_t;
			x=a->x;
			y=a->y;

			fprintf(fp,"%d;%d;%d;%4.2f;%4.2f;%4.2f;%4.2f;%4.2f;%4.2f;",pid_x.mtime,X_LOCK,x,pid_x.p,pid_x.i,pid_x.d,erro_x_t,ret_x,cont_x);

			fprintf(fp,"%d;%d;%d;%4.2f;%4.2f;%4.2f;%4.2f;%4.2f;%4.2f;\n",pid_y.mtime,Y_LOCK,y,pid_y.p,pid_y.i,pid_y.d,erro_y_t,ret_y,cont_y);

			flag_log=0;
			}

		}
	fclose(fp);
	pthread_exit((void *)NULL);
	}
void *thread_pid(void *vargp){
	PID_T pid_x,pid_y;
	int x,y,flag;
	 thread_args *a = (thread_args *) vargp;

	a->flag_log=0;

	PID_Conf(&pid_x, 0.06 , 0.000001 , 9 );//0.0004,0.00005,0);//0.5);

	PID_Conf(&pid_y, 0.06 , 0.000001 , 9  );
	while(1){
		usleep(100);
		if(flag_stop==2){
			flag_stop=3;
			break;
		}
		if(flag_pid){
			x=a->x;
			y=a->y;

			int erro_x=(X_LOCK-x);
			int erro_y=(Y_LOCK-y);

			float erro_x_t=((float)((1000.0*(float)erro_x)/(float)X_MAX));
			float erro_y_t=((float)((1000.0*(float)erro_y)/(float)Y_MAX));


			float ret_x=PID_Process(&pid_x,erro_x_t);
			float ret_y=PID_Process(&pid_y,erro_y_t);

			cont_x+=ret_x;
			cont_y+=ret_y;
			a->pid_x=pid_x;
			a->pid_y=pid_y;
			a->cont_x=cont_x;
			a->cont_y=cont_y;
			a->ret_x=ret_x;
			a->ret_y=ret_y;
			a->erro_x_t=erro_x_t;
			a->erro_y_t=erro_y_t;
			flag_log=1;

			if(cont_x>T_X_MAX) cont_x=T_X_MAX;
			if(cont_x<T_X_MIN) cont_x=T_X_MIN;

			if(cont_y>T_Y_MAX) cont_y=T_Y_MAX;
			if(cont_y<T_Y_MIN) cont_y=T_Y_MIN;
			flag_pid=0;
			flag_servo=1;
			}

		}
	pthread_exit((void *)NULL);
	}

void *thread_track(void *vargp){

	 thread_args *a = (thread_args *) vargp;
	VideoCapture cap(0); //capture the video from webcam


	// if not success, exit program
	if ( !cap.isOpened() ){
		cout << "Cannot open the web cam" << endl;
		//return -1;
		}
	int dWidth=cap.get(CV_CAP_PROP_FRAME_WIDTH);
	int dHeight=cap.get(CV_CAP_PROP_FRAME_HEIGHT);

	Size frameSize(static_cast<int>(dWidth), static_cast<int>(dHeight));
	/*VideoWriter video("./out.avi",CV_FOURCC('P','I','M','1'),20, frameSize,true);
	//namedWindow("Control", CV_WINDOW_AUTOSIZE); //create a window called "Control"
	   if ( !video.isOpened() ) //if not initialize the VideoWriter successfully, exit the program
	      {
	           cout << "ERROR: Failed to write the video" << endl;
	           //kereturn -1;
	      }*/

	int iLowH = 29;
	int iHighH = 77;

	int iLowS = 115;
	int iHighS = 255;

	int iLowV = 76;
	int iHighV = 255;

	Mat imgTmp;
	cap.read(imgTmp);

	//Create a black image with the size as the camera output
	Mat imgLines = Mat::zeros( imgTmp.size(), CV_8UC3 );;


	Mat imgOriginal;
	bool bSuccess;
	while(1){
		usleep(100);
		if(flag_stop==1){
			flag_stop=2;
			break;
		}
		bSuccess = cap.read(imgOriginal); // read a new frame from video


		//if not success, break loop
		if (!bSuccess) {
			cout << "Cannot read a frame from video stream" << endl;
			break;
			}

		Mat imgHSV;

		cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV

		Mat imgThresholded;

		inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded); //Threshold the image

		Moments oMoments = moments(imgThresholded);

		double dM01 = oMoments.m01;
		double dM10 = oMoments.m10;
		double dArea = oMoments.m00;

		// if the area <= 10000, I consider that the there are no object in the image and it's because of the noise, the area is not zero
		if (dArea > 10000){
			//calculate theake position of the ball
			int posX = dM10 / dArea;
			int posY = dM01 / dArea;
			a->x=posX;
			a->y=posY;
			flag_pid=1;

			//pthread_cond_signal(&cond);
			//circle(imgOriginal,Point(posX,posY),5,Scalar(0,0,255),2,8,0);
			//circle(imgOriginal,Point(X_LOCK,Y_LOCK),5,Scalar(0,0,255),2,8,0);

			if((posX<(X_LOCK+50)) & (posX>(X_LOCK-50)) & (posY<(Y_LOCK+50)) & (posY>(Y_LOCK-50))){

				flag_led|=0x02;
				printf("lock x=%d y=%d \n",posX,posY);
			}
			else
				flag_led&=~0x02;


			flag_led|=0x01;
			printf("x=%d y=%d flag_led=%d\n",posX,posY,flag_led);
			}
		else{
			flag_led&=~0x01;
		}
		//video.write(imgOriginal);

		}
	pthread_exit((void *)NULL);
	}
void *thread_io(void *vargp){
	int version;
	if (version=gpioInitialise() < 0){
			flag_stop=1;
			pthread_exit((void *)NULL);

			}

	/*
	 if (gpioSetISRFunc(4, EITHER_EDGE, 200, isr) != 0) {
		 flag_stop=1;
		 pthread_exit((void *)NULL);
	  }
	 if (gpioSetISRFunc(4, EITHER_EDGE, 0, NULL) != 0) {
	     //return -1;
	   }*/

	gpioSetMode(servo_pin_x, PI_OUTPUT);
	//y
	gpioSetMode(servo_pin_y, PI_OUTPUT);

	gpioSetMode(LED1,PI_OUTPUT);
	gpioSetMode(LED2,PI_OUTPUT);

	gpioWrite(LED1,0);
	gpioWrite(LED2,0);

	gpioServo(servo_pin_x, (int)cont_x);
	gpioServo(servo_pin_y, (int)cont_y);

	while(1){
		usleep(100);
		if(kbhit()==0x1b){
			flag_stop=1;
			break;
		}
		if(flag_led & 0x02)
			gpioWrite(LED2,1);
		else
			gpioWrite(LED2,0);

		gpioWrite(LED1,(flag_led && 0x01));


		gpioServo(servo_pin_x, (int)cont_x);
		gpioServo(servo_pin_y, (int)cont_y);

		}

	gpioWrite(LED1,0);
	gpioWrite(LED2,0);
	gpioServo(servo_pin_x, 1500);
	gpioServo(servo_pin_y, 1500);

	gpioTerminate();

	pthread_exit((void *)NULL);
}
int main(){
	thread_args args;
	pthread_t id_track,id_pid,id_log,id_io;
	/*pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&mutex1, NULL);
	pthread_cond_init(&cond, NULL);
	pthread_cond_init(&cond1, NULL);*/

	pthread_create(&(id_track), NULL, thread_track, (void *)&(args));

	pthread_create(&(id_pid), NULL, thread_pid, (void *)&(args));

	pthread_create(&(id_log), NULL, thread_log, (void *)&(args));

	pthread_create(&(id_io), NULL, thread_io, (void *)&(args));

	pthread_join(id_io,NULL);
	printf("join io\n");
	pthread_join(id_track,NULL);
	printf("join track\n");
	pthread_join(id_pid,NULL);
	printf("join pid\n");
	pthread_join(id_log,NULL);
	printf("join log\n");


	//pthread_mutex_destroy(&mutex);
	///pthread_mutex_destroy(&mutex1);

    //pthread_cond_destroy(&cond);

	//pthread_cond_destroy(&cond1);

	// pthread_exit((void *)NULL);

	return 0;
	}
