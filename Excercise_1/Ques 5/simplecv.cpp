#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>


#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace cv;
using namespace std;

// Default resolution is 360p
#define VRES_ROWS (360)
#define HRES_COLS (640)

#define ESC_KEY (27)

// Buffer for highest resolution visualization possible
unsigned char imagebuffer[1440*2560*3]; // 1440 rows, 2560 cols/row, 3 channel

int main(int argc, char **argv)
{
    int hres = HRES_COLS;
    int vres = VRES_ROWS;
    Mat basicimage(vres, hres, CV_8UC3, imagebuffer);
    int frameCnt=0;
    Scalar clr(0,255,0); 
    Scalar ylw(0,255,255); 

    printf("hres=%d, vres=%d\n", hres, vres);

    // interactive computer vision loop 
    namedWindow("Ex1 Ques 5", CV_WINDOW_AUTOSIZE);

    // read in default image
    if(vres == 360)
    {
        basicimage = imread("Cactus360p.jpg", CV_LOAD_IMAGE_COLOR);
    }
    else if(vres == 720)
    {
        basicimage = imread("Cactus720p.jpg", CV_LOAD_IMAGE_COLOR);
    }
    else if(vres == 1080)
    {
        basicimage = imread("Cactus1080p.jpg", CV_LOAD_IMAGE_COLOR);
    }
    else if(vres == 1440)
    {
		basicimage = imread("Cactus1440p.jpg", CV_LOAD_IMAGE_COLOR);
	}
	
	if(!basicimage.data)  // Check for invalid input
    {
        printf("Could not open or find the refernece starting image\n");
        exit(-1);
    }

	resize(basicimage,basicimage,Size(),0.5,0.5,INTER_LINEAR); // resize image 180 X 320
	copyMakeBorder(basicimage,basicimage,4,4 ,4 ,4,BORDER_CONSTANT,clr); // Make the border of 4 pixel the image
	line(basicimage, Point(hres/4,0), Point(hres/4,vres), ylw, 1, LINE_8, 0); //Draw horizontal marker
	line(basicimage, Point(0,vres/4), Point(hres,vres/4), ylw, 1, LINE_8, 0); //Draw vertical marker
	
    // Create an IplImage mapping onto the basicimage Mat object
    //
    IplImage basicimageImg = basicimage;


    // Interactive LOOP
    //
    while(1)
    {
        frameCnt++;

        // Write a zero value into the image buffer
        //
        basicimageImg.imageData[frameCnt] = (unsigned char)0;

        imshow("Ex1 Ques 5", basicimage);  

        // set pace on shared memory sampling in msecs
        char c = cvWaitKey(10);

        if(c == ESC_KEY)
        {
            exit(1);
        }

    }
 
    return 1;

}
