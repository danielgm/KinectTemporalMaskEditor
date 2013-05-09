#pragma once

#include "ofMain.h"
#include "ofUtils.h"
#include "ofxCv.h"
#include "ofxKinect.h"
#include "ofxThreadedImageSaver.h"
#include "vector.h"

// For small speed improvement?
#define ONE_OVER_255 0.00392157

class testApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void exit();
	
	void initMask();
	void readMovieFrames(string);
	void readFolderFrames(string);
	void writeDistorted();
	
	void calculateDrawSize();
	
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
	
	ofVideoPlayer player;
	ofxKinect kinect;
	
	int nearThreshold;
	int farThreshold;
	int fadeRate;
	
	unsigned char* kinectPixels;
	
	vector<unsigned char*> frames;
	ofImage frame;
	
	unsigned char* maskPixels;
	unsigned short int* maskPixelsDetail;
	ofPixels maskOfp;
	ofImage mask;
	
	unsigned char* distortedPixels;
	ofxThreadedImageSaver distorted;
	
	int frameCount;
	int frameWidth;
	int frameHeight;
	
	int screenWidth;
	int screenHeight;
	
	int drawWidth;
	int drawHeight;
	
	bool maskInitialized;
	bool gotKinectFrame;
	bool showMask;
	bool reverseTime;
	bool recording;
};
