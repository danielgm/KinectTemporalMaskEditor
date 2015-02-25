#pragma once

#include "ofMain.h"
#include "ofxKinect.h"
#include "ofxXmlSettings.h"
#include "FastBlurrer.h"

class ofApp : public ofBaseApp{

  public:
    void setup();
    void update();
    void draw();

    void saveFrame();
    void loadSettings();
    void writeSettings();

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);

private:
  ofxKinect kinect;
  FastBlurrer blurrer;

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

  ofImage drawImage;

	bool showHud;
	bool showMask;
	bool showGhost;
	bool recording;
	bool loading;

  ofTrueTypeFont hudFont;

	string recordingPath;
	int recordingImageIndex;

  int blurAmount;
};
