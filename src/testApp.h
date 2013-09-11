#pragma once

#include "ofMain.h"
#include "ofUtils.h"
#include "ofxCv.h"
#include "ofxKinect.h"
#include "time.h"
#include "vector.h"

class testApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void exit();
	
	void countFrames(string path);
	void loadFrames(string path, unsigned char* pixels);
	void writeDistorted();
	void calculateDrawSize(string path);
	
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
	
	/** Input frames. frameCount x frameWidth x frameHeight x 4 */
	unsigned char* inputPixels;
	
	/** Use blurred mask to select input pixels. frameWidth x frameHeight x 4 */
	unsigned char* outputPixels;
	
	int frameCount;
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
