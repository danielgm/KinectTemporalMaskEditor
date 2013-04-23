#pragma once

#include "ofMain.h"
#include "ofUtils.h"
#include "ofxCv.h"
#include "ofxKinect.h"
#include "vector.h"

// For small speed improvement?
#define ONE_OVER_255 0.00392157

class testApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void exit();
	
	void clearFrames();
	void readFrames(string);
	void writeDistorted();
	
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
	
	unsigned char* tempPixels;
	
	vector<unsigned char*> frames;
	ofImage frame;
	bool gotKinectFrame;
	
	unsigned char* maskPixels;
	unsigned short int* maskPixelsDetail;
	ofPixels maskOfp;
	ofImage mask;
	
	unsigned char* distortedPixels;
	ofImage distorted;
	
	int frameCount;
	int frameWidth;
	int frameHeight;
	
	bool showMask;
};
