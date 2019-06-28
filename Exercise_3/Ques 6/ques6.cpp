/*
 * @ques6.cpp: This file contains c++ code to track the laser pointer from a video with clutter and remove 
 * the background, mark the contour, overlay crosshair on the original video to mark the COM of the laser dot.
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
#define Threshold (200)
#define MAX_Threshold (255)

int main(int argc, char **argv)
{
	Mat cur_frame, prev_frame, diff_frame, gray_frame, thresh_frame;
	int c = 1;
	int thres = 0;
	string filename;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	Mat drawing;
	Scalar color_rect = Scalar(255,255,255);
	Scalar color_cont = Scalar(255,0, 0);
	int max;
	
	VideoCapture vid("Light-Room-Laser-Spot-with-Clutter.mpeg");
	
	if(!vid.isOpened())      // Check if video file opened successfully
	{
		cout << "Error opening video file" << endl;
		return -1;
	}
    
    //Initial start for difference
    vid.read(cur_frame);
    extractChannel(cur_frame, prev_frame, Green_channel);        //Extract green channel from BGR image. i.e conversion to grayscale in G band
    diff_frame = cur_frame.clone();
       
    do
    {
 		vid.read(cur_frame);   // Capture frame-by-frame
  
		// If the frame is empty, break immediately
		if (cur_frame.empty())
			break;
	
		extractChannel(cur_frame, gray_frame, Green_channel);        //Extract green channel from BGR image. i.e conversion to grayscale in G band
		absdiff(prev_frame, gray_frame, diff_frame);
		blur(diff_frame, diff_frame, Size(3,3)); 
		
		thres = 0;
		
		do    														//Successively decrease the threashold in step size of 50 bins and check till something is found
		{
			threshold(diff_frame, thresh_frame, Threshold - thres, MAX_Threshold, THRESH_BINARY);    //Apply threshold on gray scale image
			thres += 50;
		}while((countNonZero(thresh_frame) <= 15) && (thres < 200));
		
		findContours(thresh_frame, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE, Point(0, 0));     //Finds only the external contours and not the nested ones
		
		vector<vector<Point> > contours_poly( contours.size() );
		vector<Rect> boundRect(contours.size());
		vector<Moments> mu(contours.size());
		vector<Point2f> mc(contours.size());
		 
		max = 0;
		
		for(size_t i = 0; i < contours.size(); i++ )
		{
			approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true); 		//Find approximate polygon around the closed contour with minimum vertices
			boundRect[i] = boundingRect( Mat(contours_poly[i]));		//Find the bounded rectangle. This will always be parallel to images and not the detected contour so may not be of minimum area
			mu[i] = moments( contours[i], true);						//Find moments
			mc[i] = Point2f( mu[i].m10/mu[i].m00 , mu[i].m01/mu[i].m00 );    //Find center of mass of detected contour from moment
			
			if(mu[i].m00 > mu[max].m00) 									//Find the index of contour with maximum area
				max = i;
		}
		
		drawing = Mat::zeros(thresh_frame.size(), CV_8UC3);
		
		//insertChannel(thresh_frame, drawing, Green_channel);	//Insert threshold gray scale image in green channel. This will make the laser dot green in color.
		cur_frame.copyTo(drawing);		//Draw on original BGR video
		
		for(size_t i = 0; i< contours.size(); i++ )
		{
			if(i == max)
			{
				drawContours(drawing, contours_poly, (int)i, color_cont, 1, 8, vector<Vec4i>(), 0, Point());    //Draw contour in Blue color
				rectangle( drawing, boundRect[i].tl(), boundRect[i].br(), color_rect, 1, 8, 0 );	//Draw rectangle in White color
				line(drawing, Point(mc[i].x - 50, mc[i].y), Point(mc[i].x + 50, mc[i].y), color_rect, 1, LINE_8, 0);  //Draw horizontal marker intersecting COM of contoue
				line(drawing, Point(mc[i].x, mc[i].y - 50), Point(mc[i].x, mc[i].y + 50), color_rect, 1, LINE_8, 0);  //Draw vertical marker intersecting COM of contoue
			}
		}
					
		filename = "./Track_frame" + to_string(c) + ".ppm";
		cout << filename << endl;
		imwrite(filename.c_str(), drawing);
		prev_frame = gray_frame.clone();
		c++; 
	}while(1);
	
	system("ffmpeg -r 30 -s 1920x1080 -i Track_frame%d.ppm out.mpeg");
}

