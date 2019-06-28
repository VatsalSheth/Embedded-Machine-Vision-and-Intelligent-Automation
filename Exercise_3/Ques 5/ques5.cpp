/*
 * @ques5.cpp: This file contains c++ code to track the laser pointer from a video without clutter 
 * and mark the contour, overlay crosshair on the original video to mark the COM of the laser dot.
 * @author: Vatsal Sheth
 * @date: 6/27/19
 * @reference OpenCv Documenation and tutorials: 
 * https://docs.opencv.org/3.3.0/da/d0c/tutorial_bounding_rects_circles.html
 * https://docs.opencv.org/3.0-beta/doc/tutorials/imgproc/shapedescriptors/moments/moments.html
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
#define Threshold (250)
#define MAX_Threshold (255)

int main(int argc, char **argv)
{
	Mat cur_frame, gray_frame, thresh_frame;
	int c = 1;
	string filename;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	Mat drawing;
	Scalar color_rect = Scalar(255,255,255);
	Scalar color_cont = Scalar(255,0, 0);
		
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
			
		extractChannel(cur_frame, gray_frame, Green_channel);        //Extract green channel from BGR image. i.e conversion to grayscale in G band
		threshold(gray_frame, thresh_frame, Threshold, MAX_Threshold, THRESH_BINARY);    //Apply threshold on gray scale image
		
		findContours(thresh_frame, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE, Point(0, 0));
		
		vector<vector<Point> > contours_poly( contours.size() );
		vector<Rect> boundRect(contours.size());
		vector<Moments> mu(contours.size());
		vector<Point2f> mc(contours.size());
		 
		for( size_t i = 0; i < contours.size(); i++ )
		{
			approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true);
			boundRect[i] = boundingRect( Mat(contours_poly[i]));
			mu[i] = moments( contours[i], true);
			mc[i] = Point2f( mu[i].m10/mu[i].m00 , mu[i].m01/mu[i].m00 );
		}
		
		drawing = Mat::zeros(thresh_frame.size(), CV_8UC3);
		
		//insertChannel(thresh_frame, drawing, Green_channel);	//Insert threshold gray scale image in green channel. This will make the laser dot green in color.
		cur_frame.copyTo(drawing);		//Draw on original BGR video
		
		for( size_t i = 0; i< contours.size(); i++ )
		{
			drawContours(drawing, contours_poly, (int)i, color_cont, 1, 8, vector<Vec4i>(), 0, Point());    //Draw contour in Blue color
			rectangle( drawing, boundRect[i].tl(), boundRect[i].br(), color_rect, 2, 8, 0 );	//Draw rectangle in White color
			line(drawing, Point(mc[i].x - 50, mc[i].y), Point(mc[i].x + 50, mc[i].y), color_rect, 1, LINE_8, 0);  //Draw horizontal marker intersecting COM of contoue
			line(drawing, Point(mc[i].x, mc[i].y - 50), Point(mc[i].x, mc[i].y + 50), color_rect, 1, LINE_8, 0);  //Draw vertical marker intersecting COM of contoue
		}
				
		filename = "./Track_frame" + to_string(c) + ".ppm";
		cout << filename << endl;
		imwrite(filename.c_str(), drawing); 
		c++; 
	}while(1);

	system("ffmpeg -r 30 -s 1920x1080 -i Track_frame%d.ppm out.mpeg");
}
