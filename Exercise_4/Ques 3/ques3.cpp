 /*
 * @ques3.cpp: This file contains c++ code to subtact successive frames in a video to eliminate background and generate 
 * an MPEG resultant video. 
 * @author: Vatsal Sheth
 * @date: 6/25/19
 * @reference OpenCv Documenation and tutorials
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

Mat skeletal(Mat src)
{
	Mat gray, binary, mfblur;
	  
	cvtColor(src, gray, CV_BGR2GRAY);
	
	// Use 70 negative for Moose, 150 positive for hand
	// 
	// To improve, compute a histogram here and set threshold to first peak
	//
	// For now, histogram analysis was done with GIMP
	threshold(gray, binary, 120, 255, CV_THRESH_BINARY_INV);
	
	// To remove median filter, just replace blurr value with 1
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

	namedWindow("Capture Example", CV_WINDOW_AUTOSIZE);

    while(1)
    {
        frame=cvQueryFrame(capture);
		if(!frame) break;

		ip = cvarrToMat(frame);
		op = ip;
		op = skeletal(ip);
        
       	filename = "./frame" + to_string(c) + ".jpeg";
		cout << filename << endl;
		imwrite(filename.c_str(), op);
		
		if(c >= 3000)
			break;
			
		c++;
    }
	
	cvReleaseCapture(&capture);
    cvDestroyWindow("Capture Example");
    
	system("ffmpeg -r 30 -i frame%d.jpeg out.mpeg"); 
} 
