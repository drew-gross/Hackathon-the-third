
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

    // locate haar cascade from inside application bundle
    // (this is the mac way to package application resources)
    CFBundleRef mainBundle  = CFBundleGetMainBundle ();
    assert (mainBundle);
    CFURLRef    cascade_url = CFBundleCopyResourceURL (mainBundle, CFSTR("haarcascade_frontalface_alt2"), CFSTR("xml"), NULL);
    assert (cascade_url);
    Boolean     got_it      = CFURLGetFileSystemRepresentation (cascade_url, true, 
                                                                reinterpret_cast<UInt8 *>(CASCADE_NAME), CASCADE_NAME_LEN);
    if (! got_it)
        abort ();
    
    // create all necessary instances
    cvNamedWindow (WINDOW_NAME, CV_WINDOW_AUTOSIZE);
	cvNamedWindow ("Greyscale", CV_WINDOW_AUTOSIZE);
    CvCapture * camera = cvCreateCameraCapture (CV_CAP_ANY);
    CvHaarClassifierCascade* cascade = (CvHaarClassifierCascade*) cvLoad (CASCADE_NAME, 0, 0, 0);
    CvMemStorage* storage = cvCreateMemStorage(0);
    assert (storage);

    // you do own an iSight, don't you ?!?
    if (! camera)
        abort ();

    // did we load the cascade?!?
    if (! cascade)
        abort ();

    // get an initial frame and duplicate it for later work
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
	bool disabled = false;
	int numRecs = 0;
	int avgRecs = 0;
	int frames = 0; int frameCountTime = time(NULL);
	
	printf("Current frame size: (%d, %d)\n", current_frame->width, current_frame->height);
    
    // as long as there are images ...
    while (current_frame = cvQueryFrame (camera))
    {
		/*
		 CVAPI(void) cvSmooth( const CvArr* src, CvArr* dst,
		 int smoothtype CV_DEFAULT(CV_GAUSSIAN),
		 int size1 CV_DEFAULT(3),
		 int size2 CV_DEFAULT(0),
		 double sigma1 CV_DEFAULT(0),
		 double sigma2 CV_DEFAULT(0));
		 */
		cvResize(current_frame, small_image, CV_INTER_LINEAR);
		//cvSmooth(current_frame, draw_image, CV_GAUSSIAN, 5, 5);
		
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
		
		//cvDilate(gray_image, gray_image, 0, 18);
		//cvErode(gray_image, gray_image, 0, 10);
		
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
						cvPoint(current_frame->width * 0.20, current_frame->height * 0.30),
						CV_RGB(0, 255, 0), 1);
			cvRectangle(current_frame,
						cvPoint(current_frame->width * 0.80, 0),
						cvPoint(current_frame->width, current_frame->height * 0.30),
						CV_RGB(0, 255, 0), 1);
			
			if ((pt2.x < current_frame->width * 0.20 || pt1.x > current_frame->width * 0.80) &&
					pt2.y < current_frame->height * 0.30) {
				numRecs += bndRect.width * bndRect.height * scale * scale;
				cvRectangle(current_frame, pt1, pt2, CV_RGB(255, 0, 0), 1);
				//system("osascript -e \"tell application \\\"Microsoft Excel\\\" to activate\"");
			}
		}
		avgRecs = (avgRecs + numRecs) / 2.0;
		if (avgRecs > 600 && !disabled) {
			printf("%d, %d\n", avgRecs, numRecs);
			system("osascript -e \"tell application \\\"Microsoft Excel\\\" to activate\"");
			cvWaitKey (10000);
			disabled = true;
		} else {
			disabled = false;
		}
		
		++frames;
		
		if (frameCountTime != time(NULL)) {
			frameCountTime = time(NULL);
			printf("FPS: %d\n", frames);
		}
		
        /* // convert to gray and downsize
		 cvCvtColor (current_frame, gray_image, CV_BGR2GRAY);
		 cvResize (gray_image, small_image, CV_INTER_LINEAR);
		 
		 // detect faces
		 CvSeq* faces = cvHaarDetectObjects (small_image, cascade, storage,
		 1.1, 2, CV_HAAR_DO_CANNY_PRUNING,
		 cvSize (30, 30));
		 
		 // draw faces
		 cvFlip (current_frame, draw_image, 1);
		 for (int i = 0; i < (faces ? faces->total : 0); i++)
		 {
		 CvRect* r = (CvRect*) cvGetSeqElem (faces, i);
		 CvPoint center;
		 int radius;
		 center.x = cvRound((small_image->width - r->width*0.5 - r->x) *scale);
		 center.y = cvRound((r->y + r->height*0.5)*scale);
		 radius = cvRound((r->width + r->height)*0.25*scale);
		 cvCircle (draw_image, center, radius, CV_RGB(0,255,0), 3, 8, 0 );
		 }*/
        
        // just show the image
        cvShowImage (WINDOW_NAME, current_frame);
		//cvShowImage ("Greyscale", gray_image);
        
        // wait a tenth of a second for keypress and window drawing
        if (!disabled) {
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
