#pragma once

#include "ofMain.h"
#include "ofUtils.h"
#include "ofxCv.h"
#include "ofxKinect.h"
#include "time.h"
#include "vector.h"

struct inputClip {
	string title;
	string path;
	string credit;
};

class testApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void exit();
	
	void initMask();
	inputClip addInputClip(string title, string path, string credit);
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
	float kinectAngle;
	
	unsigned char* maskPixels;
	unsigned short int* maskPixelsDetail;
	ofPixels maskOfp;
	ofImage mask;
	
	ofPixels ghostOfp;
	unsigned char* ghostPixels;
	
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
	bool showGhost;
	bool reverseTime;
	bool recording;
	
	vector<inputClip> inputClips;
	string credit;
	unsigned char* inputPixels;
	
	ofTrueTypeFont hudFont;
	ofTrueTypeFont messageFont;
	ofTrueTypeFont creditFont;
	
	string recordingPath;
	int recordingImageIndex;
};
