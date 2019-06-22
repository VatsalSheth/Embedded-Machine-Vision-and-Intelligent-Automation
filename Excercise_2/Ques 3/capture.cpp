 /*
 * @capture.cpp: This file contains c++ code for continuous capture from camera. 
 * It also performs logging of time measurement and other frame calculation.
 * @author: Vatsal Sheth
 * @date: 6/20/19
 * @reference: Simpler_capture code by Prof Sam Siewert. Frame rate calculation code by Vatsal Sheth from RTES assignment 4.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <syslog.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;
using namespace std;

#define NSEC_PER_SEC (1000000000)
#define Deadline 0.08

int frame_cnt = 0;
double fps = 0;
double max_time = 0;
double jitter = 0;

int delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t)
{
  int dt_sec=stop->tv_sec - start->tv_sec;
  int dt_nsec=stop->tv_nsec - start->tv_nsec;

  if(dt_sec >= 0)
  {
    if(dt_nsec >= 0)
    {
      delta_t->tv_sec=dt_sec;
      delta_t->tv_nsec=dt_nsec;
	  return(1);
    }
    else
    {
      delta_t->tv_sec=dt_sec-1;
      delta_t->tv_nsec=NSEC_PER_SEC+dt_nsec;
	  if(dt_sec-1>=0)
		   return(1);
	   else
		   return(0);
    }
  }
  else
  {
    if(dt_nsec >= 0)
    {
      delta_t->tv_sec=dt_sec;
      delta_t->tv_nsec=dt_nsec;
    }
    else
    {
      delta_t->tv_sec=dt_sec-1;
      delta_t->tv_nsec=NSEC_PER_SEC+dt_nsec;
    }
	return(0);
  }
}

void frame_rate(struct timespec *delta_t)
{
	double delta_sec, inst_jitter; 
	
	delta_sec = (double)delta_t->tv_sec+((double)delta_t->tv_nsec/(double)NSEC_PER_SEC);
	fps = (fps*(double)frame_cnt + 1.0/delta_sec)/((double)frame_cnt+1.0);
	
	inst_jitter = (Deadline - delta_sec);
	jitter = (jitter*(double)frame_cnt + inst_jitter)/((double)frame_cnt+1.0);
	
	if(delta_sec > max_time)
		max_time = delta_sec;

	syslog(LOG_INFO, "Frame %f: Execution time: %f sec and Inst Jitter: %f sec",(double)frame_cnt, delta_sec, inst_jitter);
}

int main( int argc, char** argv )
{
    struct timespec start_time,end_time,delta_time;	
    cvNamedWindow("Capture Example", CV_WINDOW_AUTOSIZE);
    CvCapture* capture = cvCreateCameraCapture(0);
    IplImage* frame;
    char q;
 
    openlog("Ques3",LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER );

    while(1)
    {
        clock_gettime(CLOCK_REALTIME, &start_time);

        frame=cvQueryFrame(capture);
     
        if(!frame) break;

        cvShowImage("Capture Example", frame);

	clock_gettime(CLOCK_REALTIME, &end_time);		
	delta_t(&end_time, &start_time, &delta_time);
		
	if(frame_cnt)
		frame_rate(&delta_time);
		
	frame_cnt++;
	if(frame_cnt==1800)
		break;
      	
      	char q = cvWaitKey(1);
        if(q == 'q')
        {
        	printf("Quit\n"); 
            break;
        }
    }

    syslog(LOG_INFO, "Average fps: %f",fps);
    syslog(LOG_INFO, "Worst Case Frame execution time: %f sec",max_time);
    syslog(LOG_INFO, "Average Jitter: %f sec",jitter);
    
    cvReleaseCapture(&capture);
    cvDestroyWindow("Capture Example");
    
};
