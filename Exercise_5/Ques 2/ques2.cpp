/*
 * @ques2.cpp: This file contains c++ code to detect lane and vehicles and mark them on original frames to generate
 * modified video.
 * @author: Vatsal Sheth
 * @date: 7/26/19
 * @reference OpenCv Documenation and tutorials: 
 * https://medium.com/@mrhwick/simple-lane-detection-with-opencv-bfeb6ae54ec0
 * https://github.com/tomazas/opencv-lane-vehicle-track/
 * https://github.com/andrewssobral/vehicle_detection_haarcascades/
 */
 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/objdetect.hpp"

using namespace cv;
using namespace std;

double max_time = 0;
double avg_time = 0;

bool show_flag = false;
bool store_flag = false;

Mat lane_mask, lane_frame, cur_frame, op_frame;
int height_offset;
String car_cascade_name = "cars.xml";
CascadeClassifier car_cascade;

#define NSEC_PER_SEC (1000000000)

//Calculate time difference in nano scale resolution
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

//Calculate execution time and track max value 
void frame_rate(struct timespec *delta_t)
{
	double delta_sec; 
	
	delta_sec = (double)delta_t->tv_sec+((double)delta_t->tv_nsec/(double)NSEC_PER_SEC);
	avg_time += delta_sec;
		
	if(delta_sec > max_time)
		max_time = delta_sec;
}

//Generate triangular mask for lane detection based on frame dimensions
void create_lane_mask(Mat src)
{
	Point tri_mask[1][3];
	int num = 3;
	
	lane_mask = Mat::zeros(Size(src.cols, src.rows/2), CV_8U);
	
	//Points for Triangular mask vertices 
	tri_mask[0][0] = Point(src.cols/2, 0);
	tri_mask[0][1] = Point(0, src.rows/2);
	tri_mask[0][2] = Point(src.cols, src.rows/2);
	
	const Point* corner_list[1] = {tri_mask[0]};
	
	cv::fillPoly(lane_mask, corner_list, &num, 1, 255, 8);	//Create white triangular mask on black background
	
	height_offset = src.rows/2;				//Height offset to draw lane marking from cropped image to original image
}

//Detect car using Haar cascade classifier and mark a rectangle surrounding it on original frame
void detect_car()
{
	Mat src, gray_frame;
	std::vector<Rect> cars;
	
	cur_frame.copyTo(src);
	
	car_cascade.detectMultiScale(src, cars, 1.2, 3, CASCADE_DO_CANNY_PRUNING, Size(0, 0));		//Detect car feature from frame
	
	for( size_t i = 0; i < cars.size(); i++ )			//Draw rectangle surrounding detected feature
    {
		cv::rectangle(lane_frame, cars[i], Scalar(255, 0, 255), 2, 8, 0);
    }
    
    lane_frame.copyTo(op_frame);
}

//Detect lane and mark them on original frame
void detect_lane()
{
	Mat crop_frame, mask_frame, canny_frame, gray_frame, src;
	Rect roi; 
	double slope;
	
	cur_frame.copyTo(src);
	
	roi.x = 0;
	roi.y = src.rows/2;
	roi.width = src.cols;
	roi.height = src.rows/2;
	
	crop_frame = src(roi);     //Crop frame to half
	
	cvtColor(crop_frame, gray_frame, CV_BGR2GRAY); //Convert to gray scale
	blur(gray_frame, gray_frame, Size(3,3));	//Apply gaussian blur
	Canny(gray_frame, canny_frame, 100, 200, 3);	//Detect edges using canny edge transform
	
	
	canny_frame.copyTo(mask_frame, lane_mask);   //Apply the Triangular mask to detect lines only from region of interest
	
	if(show_flag == 1)
		imshow("Lane detection: Edges on ROI", mask_frame);
	
	vector<Vec4i> lines;
	HoughLinesP(mask_frame, lines, 1, CV_PI/180, 150, 100, 10);		//Detect lines using probabilistic hough transform 
			
	for( size_t i = 0; i < lines.size(); i++ )
	{
		Vec4i l = lines[i];
		slope = (l[3]-l[1])/((double)(l[2]-l[0]));				//calculate slope to remove almost horizontal lines from this list
		if(slope>0.5 || slope<(-0.5))	                        //Draw only those lines which satisfy this slope criteria
		{
			line(src, Point(l[0], l[1]+height_offset), Point(l[2], l[3]+height_offset), Scalar(0,0,255), 3, CV_AA);	 //Add vertical offset to draw line on original full size frame
		}
	}
	src.copyTo(lane_frame);
}

//Load cascade database xml file
int initialize_cascade()
{
	if(!car_cascade.load(car_cascade_name))
	{ 
		cout<<"Fail to load car cascade xml file !!!\n"; 
		return -1; 
	}
}

int main(int argc, char **argv)
{
	int c = 1;
	string filename;
	string ffmpeg_cmd;
	
	struct timespec start_time,end_time,delta_time;
	
	const char *keys =
            "{ help h usage ? || Print this message }"
            "{ @i              |<none>| Input video filename }"
            "{ show           || Show intermediate results }"
            "{ @store          |<none>| Output video filename }";
            
    CommandLineParser parser(argc, argv, keys);
    parser.about("Lane Departure Warning System");
    
    String ip_video = parser.get<String>("@i");
        
    if (parser.has("help") || (!parser.has("@i")))
    {
        parser.printMessage();
        return -1;
	}
	
	if(parser.has("show"))
	{
		show_flag = 1;
	}
		
	if(parser.has("@store"))
	{
		store_flag = 1;
		String op_video = parser.get<String>("@store");
		
		if(op_video.empty())
		{
			parser.printMessage();
			return -1;
		}
		ffmpeg_cmd = "ffmpeg -r 30 -i Track_frame%d.ppm " + op_video;
	}
		
	VideoCapture vid(ip_video.c_str());

	if(!vid.isOpened())      // Check if video file opened successfully
	{
		cout << "Error opening video file" << endl;
		return -1;
	}
    
    vid >> cur_frame;    // Capture frame-by-frame
	create_lane_mask(cur_frame);		//Generate lane mask from first frame dimension
	
	if(initialize_cascade()== -1)
		return -1;
	
    do
    {
		vid >> cur_frame;    // Capture frame-by-frame
  
		// If the frame is empty, break immediately
		if (cur_frame.empty())
			break;
		
		clock_gettime(CLOCK_REALTIME, &start_time);
		
		detect_lane();
		detect_car();
		
		clock_gettime(CLOCK_REALTIME, &end_time);		
		delta_t(&end_time, &start_time, &delta_time);
		
		if(c)
			frame_rate(&delta_time);
				
		if(show_flag == 1)
		{
			imshow("Final output", op_frame);
			cvWaitKey(1);
		}	
		
		if(store_flag == 1 && c>750)
		{
			filename = "./Track_frame" + to_string(c-750) + ".ppm";
			//cout << filename << endl;
			imwrite(filename.c_str(), op_frame); 
		}
		if(c>1750)
			break;
		c++; 
	}while(1);
	
	if(store_flag == 1)
		system(ffmpeg_cmd.c_str());				//Generate ouyput video using ffmpeg
	
	avg_time = avg_time / c;			//Calculate average execution time
	printf("Average execution time is %f sec and Max execution time is %f sec\n", avg_time, max_time);
}
