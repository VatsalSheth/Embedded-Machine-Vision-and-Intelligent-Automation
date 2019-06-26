 /*
 * @ques2.cpp: This file contains c++ code to extract 1st frame from a video, convert it to gray scale in G band 
 * and apply median filter on it. It displays before and after images and also stores both.
 * @author: Vatsal Sheth
 * @date: 6/25/19
 */
 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

#define HRES 1920
#define VRES 1080
#define ESC_KEY (27)
#define Green_channel (1)
#define kernel_size (3)

int main(int argc, char **argv)
{
	Mat ip_image, op_image; 
	char c;

	system("ffmpeg -i Dark-Room-Laser-Spot.mpeg -vframes 1 frame.ppm -hide_banner");

	namedWindow("Frame before median filter", CV_WINDOW_AUTOSIZE);
	namedWindow("Frame after median filter", CV_WINDOW_AUTOSIZE);
	
	ip_image = imread("frame.ppm", CV_LOAD_IMAGE_COLOR);
	if(!ip_image.data)  // Check for invalid input
	{
	        printf("Could not open image\n");
	        exit(-1);
	}

	imshow("Frame before median filter", ip_image);  
	
	extractChannel(ip_image, op_image, Green_channel);        //Extract green channel from BGR image. i.e conversion to grayscale in G band
	medianBlur(op_image, op_image, kernel_size);              //Median filter withh 3x3 kernel size
	
	imshow("Frame after median filter", op_image);  
	imwrite( "./Modified_frame.ppm", op_image );			//Store the modified image

	while(1)
    {
		c = cvWaitKey(10);
	    if(c == ESC_KEY)
	    {
			exit(0);
	    }
	}
}

