 #include <io.h>
#include <sstream>
#include <string>
#include <iostream>
#include <opencv\highgui.h>
#include <opencv\cv.h>
#include <math.h>


#include "libwebsockets.h"
#include "rapidjson/document.h"		
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <fstream>

using namespace cv;

int H_MIN = 0;
int H_MAX = 255;
int S_MIN = 0;
int S_MAX = 255;
int V_MIN = 0;
int V_MAX = 255;

// green 
int H_MIN1 = 42;
int H_MAX1 = 255;
int S_MIN1 = 45;
int S_MAX1 = 75;
int V_MIN1 = 145;
int V_MAX1 = 255;

// yellow 
int H_MIN2 = 20;
int H_MAX2 = 70;
int S_MIN2 = 25;
int S_MAX2 = 255;
int V_MIN2 = 105;
int V_MAX2 = 255;

// blue 
int H_MIN3 = -1;
int H_MAX3 = 110;
int S_MIN3 = 75;
int S_MAX3 = 120;
int V_MIN3 = 160;
int V_MAX3 = 255;

struct ObjectFound{
	int pos[2];
	int dir[2];
	int index;
};

const int port = 9000;

const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;

const double APPROX_POLY = 15;

const int MAX_NUM_OBJECTS = 50;

const int MIN_OBJECT_AREA = 20 * 20;
const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH / 1.5;

const cv::string windowName = "Image";
const cv::string trackbarWindowName = "Trackbars";

bool trackObjects = true;
bool useMorphOps = true;

bool isTesting = false;
bool isServer = true;

std::vector<ObjectFound*> objects;

// Create json data of found objects
char* createJSON() {
	rapidjson::Document jsonDoc;
	jsonDoc.SetObject();
	rapidjson::Value myArray(rapidjson::kArrayType);
	rapidjson::Document::AllocatorType& allocator = jsonDoc.GetAllocator();

	for (int i = 0; i < objects.size(); i++)
	{
		ObjectFound* objectF = objects.at(i);
		rapidjson::Value objValue;
		objValue.SetObject();
		objValue.AddMember("posX", objectF->pos[0], allocator);
		objValue.AddMember("posY", objectF->pos[1], allocator);
		objValue.AddMember("dirX", objectF->dir[0], allocator);
		objValue.AddMember("dirY", objectF->dir[1], allocator);

		myArray.PushBack(objValue, allocator);
	}	
	jsonDoc.AddMember("objects", myArray, allocator);

	rapidjson::StringBuffer strbuf;
	rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
	jsonDoc.Accept(writer);

	const char *jsonString = strbuf.GetString();
	char * returnJSON = (char *)malloc(strlen(jsonString)*sizeof(char));
	strcpy(returnJSON, jsonString);
	return returnJSON;
}


cv::string intToString(int number){
	std::stringstream ss;
	ss << number;
	return ss.str();
}

void on_trackbar(int, void*)
{

}
// Create track bars to track color range in test mode
void createTrackbars(){

	cv::namedWindow(trackbarWindowName, 0);

	char TrackbarName[50];
	sprintf(TrackbarName, "H_MIN", H_MIN);
	sprintf(TrackbarName, "H_MAX", H_MAX);
	sprintf(TrackbarName, "S_MIN", S_MIN);
	sprintf(TrackbarName, "S_MAX", S_MAX);
	sprintf(TrackbarName, "V_MIN", V_MIN);
	sprintf(TrackbarName, "V_MAX", V_MAX);

	cv::createTrackbar("H_MIN", trackbarWindowName, &H_MIN, H_MAX, on_trackbar);
	cv::createTrackbar("H_MAX", trackbarWindowName, &H_MAX, H_MAX, on_trackbar);
	cv::createTrackbar("S_MIN", trackbarWindowName, &S_MIN, S_MAX, on_trackbar);
	cv::createTrackbar("S_MAX", trackbarWindowName, &S_MAX, S_MAX, on_trackbar);
	cv::createTrackbar("V_MIN", trackbarWindowName, &V_MIN, V_MAX, on_trackbar);
	cv::createTrackbar("V_MAX", trackbarWindowName, &V_MAX, V_MAX, on_trackbar);
}

void drawObject(int x, int y, cv::Mat &frame){
	//putText(frame, intToString(x) + "," + intToString(y), cv::Point(x, y + 30), 1, 1, cv::Scalar(0, 0, 255), 2);
	circle(frame, cv::Point(x, y), 5, cv::Scalar(0, 0, 255), 2);
}

void morphOps(cv::Mat &thresh){
	cv::Mat erodeElement = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	cv::Mat dilateElement = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(6, 6));

	erode(thresh, thresh, erodeElement);
	erode(thresh, thresh, erodeElement);

	dilate(thresh, thresh, dilateElement);
	dilate(thresh, thresh, dilateElement);
}

// Return inner angle between 2 vector using dot product
float inner2(float x2, float y2, float x3, float y3, float x1, float y1) {
	float dx21 = x2 - x1;
	float dx31 = x3 - x1;
	float dy21 = y2 - y1;
	float dy31 = y3 - y1;
	float m12 = sqrt(dx21*dx21 + dy21*dy21);
	float m13 = sqrt(dx31*dx31 + dy31*dy31);
	float theta = acos((dx21*dx31 + dy21*dy31) / (m12 * m13));
	return theta * 180 / CV_PI;
}

void findAngle(cv::Mat& frame, cv::vector<cv::Point>& poly, cv::Point& returnPoint) {
	if (poly.size() <  4) return;

	float angle = 500;
	float temp = 100;
	for (int i = 0; i < poly.size(); i++) {
		cv::Point line1, line2, pp1, pp2, pp3;
		if (i == 0) {
			pp1 = poly[poly.size() - 1];
			pp2 = poly[0];
			pp3 = poly[1];
		}
		else if (i == poly.size() - 1) {
			pp1 = poly[poly.size() - 2];
			pp2 = poly[poly.size() - 1];
			pp3 = poly[0];
		}
		else {
			pp1 = poly[i - 1];
			pp2 = poly[i];
			pp3 = poly[i + 1];
		}

		float Angle = inner2(pp1.x, pp1.y, pp3.x, pp3.y, pp2.x, pp2.y);

		if (Angle < angle) {
			returnPoint = pp2;
			angle = Angle;
		}
	}
	circle(frame, returnPoint, 5, cv::Scalar(0, 255, 0), 2);
}

void trackFilteredObject(int &x, int &y, cv::Mat threshold, cv::Mat &cameraFeed){

	cv::Mat temp;
	threshold.copyTo(temp);

	cv::vector< cv::vector<cv::Point> > contours;
	cv::vector<cv::Vec4i> hierarchy;

	findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);

	double refArea = 0;
	bool objectFound = false;
	int indexFound = -1;
	cv::Point smallestP;
	if (hierarchy.size() > 0) {
		int numObjects = hierarchy.size();

		if (numObjects<MAX_NUM_OBJECTS){
			for (int index = 0; index >= 0; index = hierarchy[index][0]) {
				cv::Moments moment = moments((cv::Mat)contours[index]);
				double area = moment.m00;

				if (area>refArea){
					x = moment.m10 / area;
					y = moment.m01 / area;
					refArea = area;
					indexFound = index;
				}
			}

			if (indexFound >= 0){
				drawObject(x, y, cameraFeed);

				cv::vector<cv::vector<cv::Point>> polyPoints(1);
				approxPolyDP(contours[indexFound], polyPoints[0], APPROX_POLY, true);
				drawContours(cameraFeed, polyPoints, 0, (255, 0, 0), 2, 8, hierarchy, 0, cv::Point());

				findAngle(cameraFeed, polyPoints[0], smallestP);

				ObjectFound *newObject = new ObjectFound();
				newObject->pos[0] = x;
				newObject->pos[1] = y;
				newObject->dir[0] = smallestP.x;
				newObject->dir[1] = smallestP.y;
				objects.push_back(newObject);
			}
		}
		else putText(cameraFeed, "TOO MUCH NOISE! ADJUST FILTER", cv::Point(0, 50), 1, 2, cv::Scalar(0, 0, 255), 2);
	}
}

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

void checkColor(const std::string &winName, cv::Mat &HSV, cv::Mat& cameraFeed, double hMin, double hMAx, double sMin, double sMax, double vMin, double vMax) {
	cv::Mat threshold;
	int x = 0, y = 0;

	inRange(HSV, cv::Scalar(hMin, sMin, vMin), cv::Scalar(hMAx, sMax, vMax), threshold);

	if (useMorphOps)
		morphOps(threshold);

	if (trackObjects)
		trackFilteredObject(x, y, threshold, cameraFeed);

	imshow(winName, threshold);
}

void processFrame(cv::Mat &cameraFeed) {
	objects.clear();
	cv::Mat HSV;
	cvtColor(cameraFeed, HSV, cv::COLOR_BGR2HSV);

	if (isTesting)
		checkColor("test", HSV, cameraFeed, H_MIN, H_MAX, S_MIN, S_MAX, V_MIN, V_MAX);
	else {
		if (H_MIN1 > 0)
			checkColor("green", HSV, cameraFeed, H_MIN1, H_MAX1, S_MIN1, S_MAX1, V_MIN1, V_MAX1);
		if (H_MIN2 > 0)
			checkColor("yellow", HSV, cameraFeed, H_MIN2, H_MAX2, S_MIN2, S_MAX2, V_MIN2, V_MAX2);
		if (H_MIN3 > 0)
			checkColor("blue", HSV, cameraFeed, H_MIN3, H_MAX3, S_MIN3, S_MAX3, V_MIN3, V_MAX3);
	}

	imshow(windowName, cameraFeed);

	if (!isServer) {
		const char* json = createJSON();
		for (int i = 0; i < strlen(json); i++){
			printf("%c", json[i]);
		}
		printf("\n");
	}

}

static int callback_http(struct libwebsocket_context * context, struct libwebsocket *wsi, enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len)
{
	return 0;
}

static int callback_dumb_increment(struct libwebsocket_context * context,
struct libwebsocket *wsi,
enum libwebsocket_callback_reasons reason,
	void *user, void *in, size_t len)
{
	switch (reason) {
	case LWS_CALLBACK_ESTABLISHED: // just log message that someone is connecting
		printf("connection established\n");
		break;
	case LWS_CALLBACK_RECEIVE: { // the funny part
		// create a buffer to hold our response
		// it has to have some pre and post padding. You don't need to care
		// what comes there, libwebsockets will do everything for you. For more info see
		// http://git.warmcat.com/cgi-bin/cgit/libwebsockets/tree/lib/libwebsockets.h#n597
		unsigned char *buf = (unsigned char*)malloc(LWS_SEND_BUFFER_PRE_PADDING + len +
			LWS_SEND_BUFFER_POST_PADDING);

		int i;

		// pointer to `void *in` holds the incomming request
		// we're just going to put it in reverse order and put it in `buf` with
		// correct offset. `len` holds length of the request.
		for (i = 0; i < len; i++) {
			buf[LWS_SEND_BUFFER_PRE_PADDING + (len - 1) - i] = ((char *)in)[i];
		}

		// log what we recieved and what we're going to send as a response.
		// that disco syntax `%.*s` is used to print just a part of our buffer
		// http://stackoverflow.com/questions/5189071/print-part-of-char-array
		printf("received data: %s, replying: %.*s\n", (char *)in, (int)len,
			buf + LWS_SEND_BUFFER_PRE_PADDING);

		// send response
		// just notice that we have to tell where exactly our response starts. That's
		// why there's `buf[LWS_SEND_BUFFER_PRE_PADDING]` and how long it is.
		// we know that our response has the same length as request because
		// it's the same message in reverse order.

		char * sendJSON = createJSON();
		int length = strlen(sendJSON);
		unsigned char* sendjson = (unsigned char*)sendJSON;
		libwebsocket_write(wsi, sendjson, length, LWS_WRITE_TEXT);

		// release memory back into the wild
		free(buf);
		break;
	}
	default:
		break;
	}

	return 0;
}

static struct libwebsocket_protocols protocols[] = {
	/* first protocol must always be HTTP handler */
	{
		"http-only",   // name
		callback_http, // callback
		0              // per_session_data_size
	},
	{
		"dumb-increment-protocol", // protocol name - very important!
		callback_dumb_increment,   // callback
		0                          // we don't use any per session data
	},
	{
		NULL, NULL, 0   /* End of list */
	}
};

int newServer() {
	// server url will be http://localhost:9000
	const char *interface1 = NULL;
	struct libwebsocket_context *context;
	// we're not using ssl
	const char *cert_path = NULL;
	const char *key_path = NULL;
	// no special options
	int opts = 0;

	struct lws_context_creation_info info;

	memset(&info, 0, sizeof info);
	info.port = port;
	info.iface = interface1;
	info.protocols = protocols;
	info.extensions = libwebsocket_get_internal_extensions();

	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;

	info.gid = -1;
	info.uid = -1;
	info.options = opts;

	// create libwebsocket context representing this server
	context = libwebsocket_create_context(&info);
	if (context == NULL) {
		fprintf(stderr, "libwebsocket init failed\n");
		return -1;
	}
	printf("starting server...\n");

	cv::VideoCapture capture;
	capture.open(0);
	capture.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
	cv::Mat cameraFeed;

	while (1) {
		libwebsocket_service(context, 50);
		// libwebsocket_service will process all waiting events with their
		// callback functions and then wait 50 ms.

		capture.read(cameraFeed);
		processFrame(cameraFeed);
		cv::waitKey(30);
	}

	libwebsocket_context_destroy(context);

	return 0;
}

int main()
{
	if (isServer)
		newServer();
	else {
		if (isTesting)
		createTrackbars();

		cv::VideoCapture capture;
		capture.open(0);
		capture.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
		capture.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);

		cv::Mat cameraFeed;

		while (1){
			capture.read(cameraFeed);
			processFrame(cameraFeed);				
			cv::waitKey(30);
		}
	}
	
	return 0;
}