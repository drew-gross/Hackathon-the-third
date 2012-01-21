
#include <OpenCV/OpenCV.h>
#include <cassert>
#include <iostream>


const char  * WINDOW_NAME  = "Face Tracker";
const CFIndex CASCADE_NAME_LEN = 2048;
      char    CASCADE_NAME[CASCADE_NAME_LEN] = "haarcascade_frontalface_alt2.xml"; // this is a dummy

using namespace std;

int main (int argc, char * const argv[]) 
{
    const int scale = 8;

    CFBundleRef mainBundle  = CFBundleGetMainBundle ();
    assert (mainBundle);
    
    cvNamedWindow (WINDOW_NAME, CV_WINDOW_AUTOSIZE);
    CvCapture * camera = cvCreateCameraCapture (CV_CAP_ANY);

    if (!camera)
        abort();

    IplImage *current_frame = cvQueryFrame (camera);
    IplImage *draw_image =
		cvCreateImage(cvSize (current_frame->width / scale, current_frame->height / scale), IPL_DEPTH_8U, 3);
    IplImage *gray_image =
		cvCreateImage(cvSize (current_frame->width / scale, current_frame->height / scale), IPL_DEPTH_8U, 1);
	IplImage *small_image =
		cvCreateImage(cvSize (current_frame->width / scale, current_frame->height / scale),
					  IPL_DEPTH_8U, 3);
	
	IplImage *moving_average =
		cvCreateImage(cvSize (current_frame->width / scale, current_frame->height / scale), IPL_DEPTH_32F, 3);
	IplImage *difference, *tmp;  
    assert (current_frame && gray_image && draw_image && small_image && moving_average);
	
	CvRect bndRect = cvRect(0,0,0,0);
	CvPoint pt1, pt2;
	bool first = true;
	int MAXSETTLINGFRAMES = 5;
	int settlingframes = 0;
	int numRecs = 0;
	int avgRecs = 0;
	int frames = 0; int frameCountTime = time(NULL);
	
	printf("Current frame size: (%d, %d)\n", current_frame->width, current_frame->height);
    
    while (current_frame = cvQueryFrame(camera)) {
		cvResize(current_frame, small_image, CV_INTER_LINEAR);
		
		if (first) {
			difference = cvCloneImage(small_image);
			tmp = cvCloneImage(small_image);
			cvConvertScale(small_image, moving_average, 1.0, 0.0);
			first = false;
		} else {
			cvRunningAvg(small_image, moving_average, 0.080, NULL);
		}
		
		cvConvertScale(moving_average, tmp, 1.0, 0.0);
		
		cvAbsDiff(small_image, tmp, difference);
		
		cvCvtColor(difference, gray_image, CV_RGB2GRAY);
		
		cvThreshold(gray_image, gray_image, 40, 255, CV_THRESH_BINARY);
		
		CvMemStorage* storage = cvCreateMemStorage(0);
		CvSeq* contour = 0;
		cvFindContours(gray_image, storage, &contour, sizeof(CvContour),
					   CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
		
		numRecs = 0;
		for ( ; contour != 0; contour = contour->h_next ) {
			bndRect = cvBoundingRect(contour, 0);
			
			pt1.x = bndRect.x * scale;
			pt1.y = bndRect.y * scale;
			pt2.x = (bndRect.x + bndRect.width) * scale;
			pt2.y = (bndRect.y + bndRect.height) * scale;
			
			cvRectangle(current_frame,
						cvPoint(0, 0),
						cvPoint(current_frame->width * 0.20, current_frame->height * 0.40),
						CV_RGB(0, 255, 0), 1);
			cvRectangle(current_frame,
						cvPoint(current_frame->width * 0.80, 0),
						cvPoint(current_frame->width, current_frame->height * 0.40),
						CV_RGB(0, 255, 0), 1);
			
			if ((pt2.x < current_frame->width * 0.20 || pt1.x > current_frame->width * 0.80) &&
					pt2.y < current_frame->height * 0.40) {
				numRecs += bndRect.width * bndRect.height * scale * scale;
				cvRectangle(current_frame, pt1, pt2, CV_RGB(255, 0, 0), 1);
			}
		}
		avgRecs = numRecs;//(avgRecs + numRecs) / 2.0;
		printf("avg=%d, num=%d, settling=%d\n", avgRecs, numRecs, settlingframes);
		if (settlingframes-- <= 0 && avgRecs > 1000) {
			system("osascript -e \"tell application \\\"Microsoft Excel\\\" to activate\"");
			cvWaitKey (10000);
			settlingframes = MAXSETTLINGFRAMES;
		}
		
		if (settlingframes < -100) settlingframes = 0;  // TODO: This is hackish, but I don't care right now.
		
		++frames;
		
		if (frameCountTime != time(NULL)) {
			frameCountTime = time(NULL);
			printf("FPS: %d\n", frames);
			frames = 0;
		}
        
        cvShowImage (WINDOW_NAME, current_frame);
        
        if (settlingframes <= 0) {
			int key = cvWaitKey (100);
			if (key == 'q' || key == 'Q')
				break;
		}
    }
	cvReleaseImage(&tmp);
	cvReleaseImage(&difference);
	cvReleaseImage(&gray_image);
	cvReleaseImage(&moving_average);
    
    // be nice and return no error
    return 0;
}
