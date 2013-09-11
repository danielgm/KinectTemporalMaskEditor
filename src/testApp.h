#pragma once

#include "ofMain.h"
#include "ofUtils.h"
#include "ofxCv.h"
#include "ofxKinect.h"
#include "time.h"
#include "vector.h"
#include "MSATimer.h"
#include "MSAOpenCL.h"

#define NUM_LAYERS 4

class testApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void exit();
	
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
	
	msa::OpenCL openCL;
	ofxKinect kinect;
	
	msa::OpenCLImage kinectDepthBuffer;
	msa::OpenCLImage maskBuffer[2];
	int activeMaskBuffer;
	msa::OpenCLImage blurredBuffer;
	msa::OpenCLImage inputBuffer[NUM_LAYERS];
	msa::OpenCLImage distortedBuffer;
	
	float nearThreshold;
	float farThreshold;
	float fadeRate;
	
	float kinectAngle;
	
	unsigned char* distortedPixels;
	unsigned char* blurredPixels;
	
	int frameCount;
	int frameWidth;
	int frameHeight;
	
	int screenWidth;
	int screenHeight;
	
	ofImage drawImage;
	int drawWidth;
	int drawHeight;
	
	bool movieFramesAllocated;
	
	bool showHud;
	bool showMask;
	bool showGhost;
	bool reverseTime;
	bool recording;
	
	ofTrueTypeFont hudFont;
	ofTrueTypeFont messageFont;
	ofTrueTypeFont submessageFont;
	
	string recordingPath;
	int recordingImageIndex;
	
	int frameOffset;
	float frameOffsetFps = 30;
	long previousTime;
	
	int blurAmount;
};
