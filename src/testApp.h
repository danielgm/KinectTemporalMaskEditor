#pragma once

#include "ofMain.h"
#include "ofUtils.h"
#include "ofxCv.h"
#include "ofxKinect.h"
#include "time.h"
#include "vector.h"

/** A layer of frames. */
struct Layer {
	int index;
	string path;
	int frameCount;

	/** For animation. */
	int frameOffset;

	// In milliseconds.
	long previousTime;

	unsigned char* pixels;

	Layer() : pixels(0) {
	}

	~Layer() {
		if (pixels) delete[] pixels;
	}
};

/** A set of layers. */
struct Set {
	int index;
	string path;
	int layerCount;

	Layer* layers;

	Set() : layers(0) {
	}

	~Set() {
		if (layers) delete[] layers;
	}
};

class testApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void exit();
	
	int countSets();
	int countLayers(Set* set);
	int countFrames(Layer* layer);
	void calculateDrawSize();
	void loadFrames(Layer* layer);
	void updateFrameOffsets(Set* set, long now);
	void writeDistorted();
	
	void fastBlur(unsigned char* pixels, int w, int h, int r);
	
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
	
	ofxKinect kinect;
	
	int nearThreshold;
	int farThreshold;
	int ghostThreshold;
	int fadeRate;
	
	float kinectAngle;
	
	/** Write kinect output to fading mask. kinect.width x kinect.height x 1 **/
	unsigned char* maskPixels;
	
	/** Write kinect to this resized image and blur. frameWidth x frameHeight x 1 */
	unsigned char* blurredPixels;
	
	/** Use blurred mask to select input pixels. frameWidth x frameHeight x 4 */
	unsigned char* outputPixels;
	
	int setCount;
	int prevSetIndex;
	int currSetIndex;
	Set* sets;
	
	// In milliseconds.
	int fadeInDuration;
	int setDuration;
	int fadeOutDuration;

	// In milliseconds.
	long setStartTime;

	int frameWidth;
	int frameHeight;
	
	int screenWidth;
	int screenHeight;
	
	ofImage drawImage;
	int drawWidth;
	int drawHeight;
	
	bool showHud;
	bool showMask;
	bool showGhost;
	bool recording;
	
	ofTrueTypeFont hudFont;
	
	string recordingPath;
	int recordingImageIndex;
	
	float frameOffsetFps = 30;
	
	int blurAmount;
};
