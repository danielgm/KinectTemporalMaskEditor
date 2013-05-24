#pragma once

#include "ofMain.h"
#include "ofUtils.h"
#include "ofxCv.h"
#include "ofxKinect.h"
#include "vector.h"

class testApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void exit();
	
	void initMask();
	void readFolderFrames(string);
	void clearMovieFrames();
	
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
	
	unsigned char* maskPixels;
	unsigned short int* maskPixelsDetail;
	ofPixels maskOfp;
	ofImage mask;
	
	unsigned char* distortedPixels;
	ofImage distorted;
	
	int frameCount;
	int frameWidth;
	int frameHeight;
	
	int screenWidth;
	int screenHeight;
	
	int drawWidth;
	int drawHeight;
	
	bool movieFramesAllocated;
	bool maskInitialized;
	
	bool showHud;
	bool showMask;
	bool reverseTime;
	bool recording;
	
	vector<string> inputNames;
	unsigned char* inputPixels;
	
	ofTrueTypeFont font;
	
	int filenameIndex;
};
