 /*
 * @ques4.cpp: This file contains c++ code to apply canny or sobel edge detction on continuous capture from camera based on user choice. 
 * It also performs logging of time measurement and other frame calculation.
 * @author: Vatsal Sheth
 * @date: 6/20/19
 * @reference: Canny and sobel transform codes by Prof Sam Siewert. Frame rate calculation code by Vatsal Sheth from RTES assignment 4.
 */
 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <syslog.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#define NSEC_PER_SEC (1000000000)

using namespace cv;
using namespace std;

#define HRES 640
#define VRES 480
#define Deadline 0.09

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

	syslog(LOG_INFO, "Frame %d: Execution time: %f sec and Inst Jitter: %f sec",frame_cnt, delta_sec, inst_jitter);
}

Mat CannyEdge(Mat ip)
{
	int lowThreshold=0;
	int kernel_size = 3;
	int ratio = 3;
	Mat canny_frame, timg_gray, timg_grad;

	cvtColor(ip, timg_gray, CV_RGB2GRAY);

	/// Reduce noise with a kernel 3x3
	blur( timg_gray, canny_frame, Size(3,3) );

	/// Canny detector
	Canny( canny_frame, canny_frame, lowThreshold, lowThreshold*ratio, kernel_size );

	/// Using Canny's output as a mask, we display our result
	timg_grad = Scalar::all(0);

	ip.copyTo(timg_grad, canny_frame);
	return timg_grad;
}

Mat SobelEdge(Mat ip)
{
	Mat grad, src_gray, grad_x, grad_y, abs_grad_x, abs_grad_y; 	/// Generate grad_x and grad_y
	
 	GaussianBlur(ip, ip, Size(3,3), 0, 0, BORDER_DEFAULT);
	cvtColor(ip, src_gray, CV_RGB2GRAY);
	
	/// Gradient X
	//Scharr( src_gray, grad_x, ddepth, 1, 0, scale, delta, BORDER_DEFAULT );
	Sobel( src_gray, grad_x, CV_16S, 1, 0, 3, 1, 0, BORDER_DEFAULT );   
	convertScaleAbs( grad_x, abs_grad_x );

	/// Gradient Y 
	//Scharr( src_gray, grad_y, ddepth, 0, 1, scale, delta, BORDER_DEFAULT );
	Sobel( src_gray, grad_y, CV_16S, 0, 1, 3, 1, 0, BORDER_DEFAULT );   
	convertScaleAbs( grad_y, abs_grad_y );
	
	/// Total Gradient (approximate)
	addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad );
	
	return grad;
}

int main(int argc, char **argv)
{
	struct timespec start_time,end_time,delta_time;	
	CvCapture* capture;
	Mat mat_frame;
	IplImage* frame;
	char timg_window_name[] = "Edge Detector Transform";       // Transform display window
	int transform = 0; 	//0 = Canny, 1 = Sobel
	char q;

	if(argc != 2)
	{
		printf("Invalid number of arguments.\n");
		return 0;
	}		
	
	if(argv[1][0] != '-')
	{
		printf("Invalid argument.\n");
		return 0;
	}
	
	if(argv[1][1] == 'c' || argv[1][1] == 'C')
	{
		transform = 0;
	}
	else if(argv[1][1] == 's' || argv[1][1] == 'S')
	{
		transform = 1;
	}
	else
	{
		printf("Invalid argument.\n");
		return 0;
	}
	
	openlog("Ques4",LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER );
   
    	namedWindow( timg_window_name, CV_WINDOW_AUTOSIZE );

    	capture = (CvCapture *)cvCreateCameraCapture(0);
    	cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, HRES);
    	cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, VRES);

    	while(1)
    	{
		clock_gettime(CLOCK_REALTIME, &start_time);
	        frame=cvQueryFrame(capture);
        	if(!frame) break;

		mat_frame = cvarrToMat(frame);
			
		if(transform == 0)
		{
			mat_frame = CannyEdge(mat_frame);
		}
		else if (transform == 1)
		{
			mat_frame = SobelEdge(mat_frame);
		}
		
		imshow(timg_window_name, mat_frame);
		
        	clock_gettime(CLOCK_REALTIME, &end_time);		
		delta_t(&end_time, &start_time, &delta_time);
			
		if(frame_cnt)
			frame_rate(&delta_time);

		frame_cnt++;
		if(frame_cnt==1800)
			break;
		
		q = cvWaitKey(1);
		if( q == 'q' )
		{
			printf("Quit\n"); 
			break;
		}
    	}

	syslog(LOG_INFO, "Average fps: %f",fps);
	syslog(LOG_INFO, "Worst Case Frame execution time: %f sec",max_time);
	syslog(LOG_INFO, "Average Jitter: %f sec",jitter);

	cvReleaseCapture(&capture);
    	cvDestroyWindow(timg_window_name);
}

