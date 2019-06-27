/*
 * @ques4.cpp: This file contains c++ code to convert successive frames in a video to gray scale in G band and generate 
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

#define HRES 1920
#define VRES 1080
#define Green_channel (1)


int main(int argc, char **argv)
{
	Mat cur_frame, temp;
	int c = 1;
	string filename;

	VideoCapture vid("Dark-Room-Laser-Spot.mpeg");

	if(!vid.isOpened())      // Check if video file opened successfully
	{
		cout << "Error opening video file" << endl;
		return -1;
	}
    
     
    do
    {
		vid >> cur_frame;    // Capture frame-by-frame
  
		// If the frame is empty, break immediately
		if (cur_frame.empty())
			break;
			
		extractChannel(cur_frame, temp, Green_channel);        //Extract green channel from BGR image. i.e conversion to grayscale in G band
		filename = "./Gray_frame" + to_string(c) + ".ppm";
		cout << filename << endl;
		imwrite(filename.c_str(), temp); //diff_frame);
		c++; 
	}while(1);

	system("ffmpeg -r 30 -s 1920x1080 -i Gray_frame%d.ppm out.mpeg");
}
