 /*
 * @ques3.cpp: This file contains c++ code to apply skeletal transform through successive thining 
 * using opencv API in top down approach.
 * @author: Vatsal Sheth
 * @date: 7/17/19
 * @reference OpenCv Documenation and tutorials, Prof Sam Siweret code for EMVIA course.
 */
 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp> 
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

double max_time = 0;
double avg_time = 0;

#define NSEC_PER_SEC (1000000000)

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
	double delta_sec; 
	
	delta_sec = (double)delta_t->tv_sec+((double)delta_t->tv_nsec/(double)NSEC_PER_SEC);
	avg_time += delta_sec;
		
	if(delta_sec > max_time)
		max_time = delta_sec;
}

Mat skeletal(Mat src)
{
	Mat gray, binary, mfblur;
	  
	cvtColor(src, gray, CV_BGR2GRAY);
	
	// Histogram analysis was done with GIMP
	threshold(gray, binary, 120, 255, CV_THRESH_BINARY_INV);
	
	// Median filter change blur value. blur = 1 means bypass
	medianBlur(binary, mfblur, 1);

	// This section of code was adapted from the following post, which was
	// based in turn on the Wikipedia description of a morphological skeleton
	//
	// http://felix.abecassis.me/2011/09/opencv-morphological-skeleton/
	//
	Mat skel(mfblur.size(), CV_8UC1, Scalar(0));
	Mat temp;
	Mat eroded;
	Mat element = getStructuringElement(MORPH_CROSS, Size(3, 3));
	bool done;
	int iterations=0;

	do
	{
		erode(mfblur, eroded, element);
		dilate(eroded, temp, element);
		subtract(mfblur, temp, temp);
		bitwise_or(skel, temp, skel);
		eroded.copyTo(mfblur);

		done = (countNonZero(mfblur) == 0);
		iterations++;
	}while(!done && (iterations < 100));

	cout << "iterations=" << iterations << endl;
	
	return skel;
}

int main( int argc, char** argv )
{
	CvCapture* capture = cvCreateCameraCapture(0);
	IplImage* frame;
	Mat ip, op;
    int c = 1;
	string filename;

	struct timespec start_time,end_time,delta_time;
	
	namedWindow("Capture Example", CV_WINDOW_AUTOSIZE);

    while(1)
    {
        frame=cvQueryFrame(capture);
		if(!frame) break;

		clock_gettime(CLOCK_REALTIME, &start_time);
		
		ip = cvarrToMat(frame);
		op = ip;
		op = skeletal(ip);
        
        clock_gettime(CLOCK_REALTIME, &end_time);		
		delta_t(&end_time, &start_time, &delta_time);
		
		if(c)
			frame_rate(&delta_time);
			
       	filename = "./frame" + to_string(c) + ".jpeg";
		cout << filename << endl;
		imwrite(filename.c_str(), op);
		
		if(c >= 3000)
		{
			avg_time = avg_time / c;
			printf("Average execution time is %f sec and Max execution time is %f sec\n", avg_time, max_time);
			break;
		}
			
		c++;
    }
	
	cvReleaseCapture(&capture);
    cvDestroyWindow("Capture Example");
    
	system("ffmpeg -r 30 -i frame%d.jpeg out.mpeg"); 
} 
