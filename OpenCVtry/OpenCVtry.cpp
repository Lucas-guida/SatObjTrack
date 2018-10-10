/////////////////////////////////////////////////////////
//Header Files
////////////////////////////////////////////////////////
#include "pch.h"

#include <sstream>
#include <string>
#include <iostream>

#include <vector>

#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp>

using namespace cv;

/////////////////////////////////////////////////////////
//Initalization Variables and Constants 
////////////////////////////////////////////////////////
//initial min and max HSV filter values.
//these will be changed using trackbars
int H_MIN = 0;
int H_MAX = 256;
int S_MIN = 0;
int S_MAX = 256;
int V_MIN = 0;
int V_MAX = 256;

//SET VALUES FOR BLUE SQUARE
int BH_MIN = 25;
int BH_MAX = 53;
int BS_MIN = 81;
int BS_MAX = 223;
int BV_MIN = 117;
int BV_MAX = 175;

//default capture width and height
const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;

//max number of objects to be detected in frame
const int MAX_NUM_OBJECTS = 8;

//minimum and maximum object area px by px?
const int MIN_OBJECT_AREA = 20 * 20;
const double MAX_OBJECT_AREA = FRAME_HEIGHT * FRAME_WIDTH / 1.5;

//names that will appear at the top of each window
const std::string windowName = "Original Image";
const std::string windowName1 = "HSV Image";
const std::string windowName2 = "Thresholded Image";
const std::string windowName3 = "After Morphological Operations";
const std::string trackbarWindowName = "Trackbars";

///////////////////////////////////////////////////
//Event Triggers
///////////////////////////////////////////////////
//Event Trigger for trackbar change. This function
//gets called  whenever a trackbar position is changed
void on_trackbar(int, void*){}

///////////////////////////////////////////////////
//Conversion Functions
//////////////////////////////////////////////////
//Function that converts an integer value to a string
std::string intToString(int number) {
	std::stringstream ss;
	ss << number;
	return ss.str();
}

//////////////////////////////////////////////////
//Calabration Code
/////////////////////////////////////////////////
//Creates the Trackbar
void createTrackbars() {
	//create window for trackbars
	namedWindow(trackbarWindowName, 0);
	
	//create trackbars and insert them into window
	//3 parameters are: the address of the variable that is changing when the trackbar is moved(eg.H_MIN),
	//the max value the trackbar can move (eg. H_MAX), 
	//and the function that is called whenever the trackbar is moved(eg. on_trackbar)     
	createTrackbar("H_MIN", trackbarWindowName, &H_MIN, H_MAX, on_trackbar);
	createTrackbar("H_MAX", trackbarWindowName, &H_MAX, H_MAX, on_trackbar);
	createTrackbar("S_MIN", trackbarWindowName, &S_MIN, S_MAX, on_trackbar);
	createTrackbar("S_MAX", trackbarWindowName, &S_MAX, S_MAX, on_trackbar);
	createTrackbar("V_MIN", trackbarWindowName, &V_MIN, V_MAX, on_trackbar);
	createTrackbar("V_MAX", trackbarWindowName, &V_MAX, V_MAX, on_trackbar);
}

////////////////////////////////////////////////////////
//Image Filtering
////////////////////////////////////////////////////////
//Create structuring elements that will be used to "dilate" and "erode" the image.
//the element chosen here is a 3px by 3px rectangle
void morphOps(Mat &thresh) {
	Mat erodeElement = getStructuringElement(MORPH_RECT, Size(3, 3));
	Mat erodeElement2 = getStructuringElement(MORPH_RECT, Size(1, 1));
	//dilate with larger element so make sure object is nicely visible
	Mat dilateElement = getStructuringElement(MORPH_RECT, Size(3, 3));
	Mat dilateElement2 = getStructuringElement(MORPH_RECT, Size(8, 8));

	erode(thresh, thresh, erodeElement);
	erode(thresh, thresh, erodeElement2);


	dilate(thresh, thresh, dilateElement);
	dilate(thresh, thresh, dilateElement2);
}

///////////////////////////////////////////////////////
//Object Tracking
///////////////////////////////////////////////////////
std::vector< std::vector<Point> > trackFilteredObject(Mat threshold, Mat &cameraFeed) {
	Mat temp;
	std::vector <std::vector<int>> objs;
	threshold.copyTo(temp);
	//these two vectors needed for output of findContours
	std::vector< std::vector<Point> > contours;
	std::vector<Vec4i> hierarchy;
	//find contours of filtered image using openCV findContours function
	findContours(temp, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);
	//use moments method to find our filtered object
	
	double refArea = 0;
	bool objectFound = false;
	if (hierarchy.size() > 0) {
		int numObjects = hierarchy.size();
		//if number of objects greater than MAX_NUM_OBJECTS we have a noisy filter
		if (numObjects < MAX_NUM_OBJECTS) {
			for (int index = 0; index >= 0; index = hierarchy[index][0]) {
				Moments moment = moments((cv::Mat)contours[index]);
				double area = moment.m00;
				//if the area is less than 20 px by 20px then it is probably just noise
				//if the area is the same as the 3/2 of the image size, probably just a bad filter
				//we only want the object with the largest area so we safe a reference area each
				//iteration and compare it to the area in the next iteration.
				//if (area > MIN_OBJECT_AREA && area<MAX_OBJECT_AREA && area>refArea) {
				if (area > MIN_OBJECT_AREA) {
					std::vector <int> object;
					object.push_back(moment.m10 / area);
					object.push_back(moment.m01 / area);
					objs.push_back(object);
					
					//x = moment.m10 / area;
					//y = moment.m01 / area;
					objectFound = true;
					//refArea = area;
				}
				else objectFound = false;

			}
			//let user know you found an object
			if (objectFound == true) {
				for (int i = 0; i < objs.size(); i++) {
					int x = objs.at(i).at(0);
					int y = objs.at(i).at(1);
					cv::circle(cameraFeed, Point(x, y), 10, Scalar(0, 255, 0), 2);
					putText(cameraFeed, intToString(i) + " " + intToString(x) + "," + intToString(y), Point(x, y + 30), 1, 1, Scalar(0, 255, 0), 2);
				}
				putText(cameraFeed, "Tracking Object", Point(0, 50), 2, 1, Scalar(0, 255, 0), 2);
			}

		}
		else putText(cameraFeed, "TOO MUCH NOISE! ADJUST FILTER", Point(0, 50), 1, 2, Scalar(0, 0, 255), 2);
	}
	return contours;
}



//////////////////////////////////////////////////////
//Main Body of Code
//////////////////////////////////////////////////////
int main(int argc, char* argv[]){
	//some boolean variables for different functionality within this
	//program
	bool trackObjects = true;
	bool useMorphOps = true;

	bool Trackbar = false;

	bool rotation = true;

	std::vector< std::vector<Point> > objectContours;
	//Matrix to store each frame of the webcam feed
	Mat cameraFeed;
	//matrix storage for HSV image
	Mat HSV;
	//matrix storage for binary threshold image
	Mat threshold;

	//create slider bars for HSV filtering
	if (Trackbar == true) {
		createTrackbars();
	}
	
	//video capture object to acquire webcam feed
	VideoCapture capture;
	//open capture object at location zero (default location for webcam)
	capture.open(0);
	//set height and width of capture frame
	capture.set(cv::CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
	capture.set(cv::CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
	//start an infinite loop where webcam feed is copied to cameraFeed matrix
	//all of our operations will be performed within this loop
	char charCheckForEscKey = 0;
	while (charCheckForEscKey != 27) {
		//store image to matrix
		capture.read(cameraFeed);
		//convert frame from BGR to HSV colorspace
		cvtColor(cameraFeed, HSV, COLOR_BGR2HSV);
		//filter HSV image between values and store filtered image to
		//threshold matrix
		if (Trackbar == true) {
			inRange(HSV, Scalar(H_MIN, S_MIN, V_MIN), Scalar(H_MAX, S_MAX, V_MAX), threshold);
		}
		else {
			inRange(HSV, Scalar(BH_MIN, BS_MIN, BV_MIN), Scalar(BH_MAX, BS_MAX, BV_MAX), threshold);
		}
		//perform morphological operations on thresholded image to eliminate noise
		//and emphasize the filtered object(s)
		if (useMorphOps)
			morphOps(threshold);
		//pass in thresholded frame to our object tracking function
		//this function will return the x and y coordinates of the
		//filtered object
		if (trackObjects) {
			objectContours = trackFilteredObject(threshold, cameraFeed);
			std::cout << objectContours.size() << std::endl;
		}


		//Canny(threshold, canny, 80, 100);

		//show frames 
		imshow(windowName2, threshold);
		imshow(windowName, cameraFeed);
		imshow(windowName1, HSV);

		//delay 30ms so that screen can refresh.
		//image will not appear without this waitKey() command
		//waitKey(30);
		charCheckForEscKey = cv::waitKey(30);
	}
	return 0;
}
