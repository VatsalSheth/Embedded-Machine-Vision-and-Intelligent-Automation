 /*
 * @ques4.cpp: This file contains c++ code to apply skeletal transform through successive thining 
 * in bottom up approach using cross number for thining.
 * @author: Vatsal Sheth
 * @date: 7/17/19
 * @reference OpenCv Documenation and tutorials, Prof Sam Siweret code for EMVIA course and Computer & Machine Vision Theory, algorithm & practicalites by E.R.Davies
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

#define P0(mat,x,y) (mat.at<uchar>(x,y))
#define P1(mat,x,y) (mat.at<uchar>(x,y+1))
#define P2(mat,x,y) (mat.at<uchar>(x-1,y+1))
#define P3(mat,x,y) (mat.at<uchar>(x-1,y))
#define P4(mat,x,y) (mat.at<uchar>(x-1,y-1))
#define P5(mat,x,y) (mat.at<uchar>(x,y-1))
#define P6(mat,x,y) (mat.at<uchar>(x+1,y-1))
#define P7(mat,x,y) (mat.at<uchar>(x+1,y))
#define P8(mat,x,y) (mat.at<uchar>(x+1,y+1))

Mat skeletal(Mat skel)
{
	// This section of code was adapted from the Computer & Machine Vision Theory, algorithm & practicalites by E.R.Davies
	bool done;
	int iterations=0;
	int sigma=0, chi=0;
	
	Mat res = skel;
		
	do
	{
		done = true;
		
		for(int j=1; j<(skel.cols-1); j++)
		{
			for(int i=1; i<(skel.rows-1); i++)
			{
				if(P0(skel,i,j) != 0)
				{
					sigma = P1(skel,i,j) + P2(skel,i,j) + P3(skel,i,j) + P4(skel,i,j) + P5(skel,i,j) + P6(skel,i,j) + P7(skel,i,j) + P8(skel,i,j);
				
					if(sigma != 255)
					{
						chi = 	(int)(P1(skel,i,j) != P3(skel,i,j)) + 
								(int)(P3(skel,i,j) != P5(skel,i,j)) + 
								(int)(P5(skel,i,j) != P7(skel,i,j)) + 
								(int)(P7(skel,i,j) != P1(skel,i,j)) +
								2*(
									(int)((P2(skel,i,j)>P1(skel,i,j)) && (P2(skel,i,j)>P3(skel,i,j))) +
									(int)((P4(skel,i,j)>P3(skel,i,j)) && (P4(skel,i,j)>P5(skel,i,j))) +
									(int)((P6(skel,i,j)>P5(skel,i,j)) && (P6(skel,i,j)>P7(skel,i,j))) +
									(int)((P8(skel,i,j)>P7(skel,i,j)) && (P8(skel,i,j)>P1(skel,i,j))) 
								);
							
						if((chi == 2))
						{
							if((P3(skel,i,j)==0) && (P7(skel,i,j)==255))  //North points
							{
								P0(res,i,j) = 0;
								done = false;
							}
							else if((P3(skel,i,j)==255) && (P7(skel,i,j)==0))	//South Points
							{
								P0(res,i,j) = 0;
								done = false;
							}
							else if((P1(skel,i,j)==0) && (P5(skel,i,j)==255))	//East Points
							{
								P0(res,i,j) = 0;
								done = false;
							}
							else if((P1(skel,i,j)==255) && (P5(skel,i,j)==0))	//West Points
							{
								P0(res,i,j) = 0;
								done = false;
							}
							
							//For direct implementaion without direction bias
							//P0(res,i,j) = 0;
							//done = false;
						}
					}
				}
			}
		}
		iterations++;
	}while(!done && (iterations < 100));

	cout << "iterations=" << iterations << endl;
	
	return res;
}


int main( int argc, char** argv )
{
	CvCapture* capture = cvCreateCameraCapture(0);
	IplImage* frame;
	Mat ip, op;
    int c = 1;
	string filename;
	Mat gray_frame, binary_frame, curr_frame, diff_frame, prev_frame;
	 
	namedWindow("Capture Example", CV_WINDOW_AUTOSIZE);

	frame=cvQueryFrame(capture);

	curr_frame = cvarrToMat(frame);
	cvtColor(curr_frame, prev_frame, CV_BGR2GRAY);
	diff_frame = curr_frame.clone();
	
    while(1)
    {
        frame=cvQueryFrame(capture);
		if(!frame) break;

		curr_frame = cvarrToMat(frame);
		
		cvtColor(curr_frame, gray_frame, CV_BGR2GRAY);
		
		//Background subtraction									//Background elimination commented for better results
		//absdiff(prev_frame, gray_frame, diff_frame);
		
		diff_frame = gray_frame;
		blur(diff_frame, diff_frame, Size(3,3)); 
		
		// Histogram analysis was done with GIMP
		threshold(diff_frame, binary_frame, 120, 255, CV_THRESH_BINARY_INV);
		
		// Median filter change blur value. blur = 1 means bypass
		medianBlur(binary_frame, ip, 3);
		
		op = ip;
		op = skeletal(ip);
        
       	filename = "./frame" + to_string(c) + ".jpeg";
		cout << filename << endl;
		imwrite(filename.c_str(), op);
		
		if(c >= 300)
			break;
			
		c++;
    }
	
	cvReleaseCapture(&capture);
    cvDestroyWindow("Capture Example");
    
	system("ffmpeg -r 30 -i frame%d.jpeg out.mpeg"); 
} 
