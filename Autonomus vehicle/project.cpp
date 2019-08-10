/*
 * @project.cpp: This file contains c++ code to detect lane, pedestrian and vehicles and mark them on original frames to generate
 * modified video.
 * @author: Vatsal Sheth & Amreeta Sengupta
 * @date: 8/9/19
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
#include <sched.h>
#include <time.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <signal.h>
#include <X11/Xlib.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/objdetect.hpp"

//#define DEBUG

#define NUM_THREADS (4)
#define NSEC_PER_SEC (1000000000)

using namespace cv;
using namespace std;

double max_time = 0;
double avg_time = 0;

bool show_flag = false;
bool store_flag = false;
bool exit_flag = false;

VideoCapture vid;
sem_t *sem_capture;

Mat lane_mask;
Rect crop_roi; 
int height_offset;
String car_cascade_name = "cars.xml";
CascadeClassifier car_cascade;
HOGDescriptor hog_pedestrian;

// POSIX thread declarations and scheduling attributes
pthread_t threads[NUM_THREADS];
pthread_attr_t thread_attr;
int max_prio;
struct sched_param thread_param;
pid_t mainpid;

String ip_video;
int frame_idx = 1;

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
	
	crop_roi.x = 0;
	crop_roi.y = src.rows/2;
	crop_roi.width = src.cols;
	crop_roi.height = src.rows/2;
}

//Detect car using Haar cascade classifier and mark a rectangle surrounding it on original frame
Mat detect_car(Mat src, Mat frame)
{
	Mat small_frame;
	std::vector<Rect> cars;
	
	resize(src, small_frame, Size(0,0), 0.5, 0.5, INTER_LINEAR );				//Down size to 50 %
	equalizeHist(small_frame, small_frame);			//Normalize histogram

	car_cascade.detectMultiScale(small_frame, cars, 1.2, 3, 0, Size(30,30));
	
	for( size_t i = 0; i < cars.size(); i++ )
    {
		cars[i].y = (cars[i].y<<1) + height_offset;
		cars[i].x = (cars[i].x<<1);
		cars[i].width = (cars[i].width<<1);
		cars[i].height = (cars[i].height<<1);
		cv::rectangle(frame, cars[i], Scalar(255, 0, 255), 2, 8, 0);
    }
    
    return frame;
}

//Detect pedestrian and mark a rectangle surrounding it on original frame
Mat detect_pedestrian(Mat src, Mat frame)
{
	Mat small_frame;
	
	resize(src, small_frame, Size(0,0), 0.5, 0.5, INTER_LINEAR );				//Down size to 50 %
	//equalizeHist(small_frame, small_frame);			//Normalize histogram
	
	vector<Rect> found;

    hog_pedestrian.detectMultiScale(small_frame, found, 0, Size(8,8), Size(32,32), 1.05, 2, false);

    // Draw bounding rectangle
    for( size_t i = 0; i < found.size(); i++ )
    {
		found[i].y = (found[i].y<<1);
		found[i].x = (found[i].x<<1);
		found[i].width = (found[i].width<<1);
		found[i].height = (found[i].height<<1);
		rectangle(frame, found[i], cv::Scalar(0,0,255), 3);
	}
	
	return frame;
}

//Detect lane and mark them on original frame
Mat detect_lane(Mat src, Mat frame)
{
	Mat mask_frame, blur_frame, canny_frame;
	double slope;
	Vec4i draw_left, draw_right;
	
	draw_left[0] = 0;
	draw_left[1] = 0;
	draw_left[2] = 0;
	draw_left[3] = crop_roi.height;
	draw_right = draw_left;
	
	blur(src, blur_frame, Size(3,3));	//Apply gaussian blur
	Canny(blur_frame, canny_frame, 100, 200, 3);	//Detect edges using canny edge transform
	
	
	canny_frame.copyTo(mask_frame, lane_mask);   //Apply the Triangular mask to detect lines only from region of interest
	
	#ifdef DEBUG
		imshow("Lane detection: Edges on ROI", mask_frame);
		cvWaitKey(1);
	#endif
	
	vector<Vec4i> lines;
	HoughLinesP(mask_frame, lines, 1, CV_PI/180, 150, 100, 10);		//Detect lines using probabilistic hough transform 
			
	for( size_t i = 0; i < lines.size(); i++ )
	{
		Vec4i l = lines[i];
		
		slope = (l[3]-l[1])/((double)(l[2]-l[0]));				//calculate slope to remove almost horizontal lines from this list
		if(slope>0.5) 	                        //Draw only those lines which satisfy this slope criteria
		{	
			if(l[1]>draw_right[1])
			{
				draw_right[1] = l[1];
				draw_right[0] = l[0];
			}
			
			if(l[3]<draw_right[3])
			{
				draw_right[2] = l[2];
				draw_right[3] = l[3];
			}		
		}
		else if(slope<(-0.5))
		{
			if(l[1]>draw_left[1])
			{
				draw_left[1] = l[1];
				draw_left[0] = l[0];
			}
			
			if(l[3]<draw_left[3])
			{
				draw_left[2] = l[2];
				draw_left[3] = l[3];
			}
		}
	}
	
	if(draw_left[0] != 0)
		line(frame, Point(draw_left[0], draw_left[1]+height_offset), Point(draw_left[2], draw_left[3]+height_offset), Scalar(0,0,255), 3, CV_AA);	 //Draw left lane. Add vertical offset to draw line on original full size frame
	
	if(draw_right[0] != 0)
		line(frame, Point(draw_right[0], draw_right[1]+height_offset), Point(draw_right[2], draw_right[3]+height_offset), Scalar(0,0,255), 3, CV_AA);	 //Draw right lane. Add vertical offset to draw line on original full size frame
	
	#ifdef DEBUG
		imshow("Lane detection: Marked", frame);
		cvWaitKey(1);
	#endif
	
	return frame;
}

//Load cascade database xml file
int initialize_cascade()
{
	if(!car_cascade.load(car_cascade_name))
	{ 
		cout<<"Fail to load car cascade xml file !!!\n"; 
		return -1; 
	}
	
	hog_pedestrian.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());
}


void *autopilot(void *threadp)
{
	string filename;
	int idx;
	Mat ip_frame, op_frame, gray_frame, crop_frame, lane_frame, ped_frame;
	
    do
    {
		sem_wait(sem_capture);
		vid >> ip_frame;    // Capture frame-by-frame
		idx = frame_idx++;
		sem_post(sem_capture);
		
		// If the frame is empty, break immediately
		if (ip_frame.empty())
			break;
		
		//clock_gettime(CLOCK_REALTIME, &start_time);
		
		cvtColor(ip_frame, gray_frame, CV_BGR2GRAY); //Convert to gray scale
		crop_frame = gray_frame(crop_roi);     //Crop frame to half
		
		lane_frame = detect_lane(crop_frame, ip_frame);
		ped_frame = detect_pedestrian(gray_frame, lane_frame);
		op_frame = detect_car(crop_frame, ped_frame);
		
		if(show_flag == 1)
		{
			imshow("Final output", op_frame);
			cvWaitKey(1);
		}	
		
		if(store_flag == 1)
		{
			filename = "./Track_frame" + to_string(idx) + ".ppm";
			//cout << filename << endl;
			imwrite(filename.c_str(), op_frame); 
		}
	}while(exit_flag==false && (frame_idx<2000));
	
	pthread_exit(NULL);
}

void signal_handler(int signo, siginfo_t *info, void *extra) 
{	
	sem_unlink("/sem_capture");
	sem_close(sem_capture);
	exit_flag = true;
}

void set_signal_handler(void)
{
	struct sigaction action;
 
    action.sa_flags = SA_SIGINFO; 
    action.sa_sigaction = signal_handler;
 
    if (sigaction(SIGINT, &action, NULL) == -1)
		perror("SIGINT: sigaction");
}

int main(int argc, char **argv)
{
	int rc, num_proc;
	string ffmpeg_cmd;
	cpu_set_t threadcpu;
	Mat tmp_frame;
	struct timespec start_time,end_time,delta_time;
	
	XInitThreads();
	
	const char *keys =
            "{ help h usage ? || Print this message }"
            "{ @i              |<none>| Input video filename }"
            "{ show           || Show intermediate results }"
            "{ @store          |<none>| Output video filename }";
            
    CommandLineParser parser(argc, argv, keys);
    parser.about("Lane Departure Warning System");
    
    ip_video = parser.get<String>("@i");
        
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
		ffmpeg_cmd = "sudo ffmpeg -r 30 -i Track_frame%d.ppm " + op_video;
	}
	
	set_signal_handler();				//Tap CTRL+C kill signal to close & unlink semaphore
	
	vid.open(ip_video.c_str());

	if(!vid.isOpened())      // Check if video file opened successfully
	{
		cout << "Error opening video file" << endl;
		return -1;
	}
    
    vid >> tmp_frame;    // Capture frame-by-frame
    
	create_lane_mask(tmp_frame);		//Generate lane mask from first frame dimension

	if(initialize_cascade()== -1)
		return -1;
	
	while(frame_idx<750)
	{
		vid >> tmp_frame; 
		frame_idx++;
	}
	frame_idx = 1;
	
	num_proc = get_nprocs(); 
	printf("Number of processors: %d\n", num_proc);
	
	sem_capture = sem_open("/sem_capture", O_CREAT,0777,1);
		
	//Main Thread
	mainpid = getpid();
	max_prio = sched_get_priority_max(SCHED_FIFO);
	rc = sched_getparam(mainpid, &thread_param);
	thread_param.sched_priority = max_prio;
	rc = sched_setscheduler(getpid(), SCHED_FIFO, &thread_param);
	if(rc < 0)
		perror("main_param");
	
	clock_gettime(CLOCK_REALTIME, &start_time);
	
	for(int i=0; i<num_proc; i++)
	{
		CPU_ZERO(&threadcpu);
		printf("Setting thread %d to core %d\n", i+1, i);
		CPU_SET(i, &threadcpu);
		rc = pthread_attr_init(&thread_attr);
		rc = pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);
		rc = pthread_attr_setschedpolicy(&thread_attr, SCHED_FIFO);
		rc = pthread_attr_setaffinity_np(&thread_attr, sizeof(cpu_set_t), &threadcpu);
		thread_param.sched_priority = max_prio-1;
		pthread_attr_setschedparam(&thread_attr, &thread_param);
	
		pthread_create(&threads[i], &thread_attr, autopilot, NULL);
	}
	
	for(rc=0; rc<num_proc; rc++)
	{
       pthread_join(threads[rc], NULL);
    }
    
    clock_gettime(CLOCK_REALTIME, &end_time);		
	delta_t(&end_time, &start_time, &delta_time);
	avg_time = (double)delta_time.tv_sec+((double)delta_time.tv_nsec/(double)NSEC_PER_SEC);
			
    sem_unlink("/sem_capture");
    sem_close(sem_capture);
    
	if(store_flag == 1)
		system(ffmpeg_cmd.c_str());				//Generate ouyput video using ffmpeg
	
	avg_time = avg_time / frame_idx;			//Calculate average execution time
	printf("Average execution time is %f sec and Max execution time is %f sec\n", avg_time, max_time);
}
