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

#define HRES 1920
#define VRES 1080

int main(int argc, char **argv)
{
	Mat cur_frame, prev_frame, diff_frame;
	int c = 1;
	string filename;
	
	VideoCapture vid("Dark-Room-Laser-Spot-with-Clutter.mpeg");
	
	if(!vid.isOpened())      // Check if video file opened successfully
	{
		cout << "Error opening video file" << endl;
		return -1;
	}
    
    //Initial start for difference
    vid.read(cur_frame);
    prev_frame = cur_frame.clone();
    diff_frame = cur_frame.clone();
       
    do
    {
 		vid.read(cur_frame);   // Capture frame-by-frame
  
		// If the frame is empty, break immediately
		if (cur_frame.empty())
			break;
	
		absdiff(cur_frame, prev_frame, diff_frame);
		filename = "./Diff_frame" + to_string(c) + ".ppm";
		cout << filename << endl;
		imwrite(filename.c_str(), diff_frame);
		prev_frame = cur_frame.clone();
		c++; 
	}while(1);
	
	system("ffmpeg -r 30 -s 1920x1080 -i Diff_frame%d.ppm out.mpeg");
}

