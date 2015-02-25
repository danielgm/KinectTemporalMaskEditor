#include "ofApp.h"

void ofApp::setup() {
  ofSetLogLevel(OF_LOG_VERBOSE);

  loadSettings();

  hudFont.loadFont("verdana.ttf", 12);

  kinect.init(true, false, false);
  kinect.open();

	ofSetFrameRate(60);
  kinect.setCameraTiltAngle(0);

  blurrer.init(kinect.width, kinect.height, 5);

  maskPixels = new unsigned char[kinect.width * kinect.height * 1];
  blurredPixels = new unsigned char[kinect.width * kinect.height * 3];
  outputPixels = new unsigned char[kinect.width * kinect.height * 3];

  // Start the mask off black.
  for (int i = 0; i < kinect.width * kinect.height; i++) {
    maskPixels[i] = 0;
  }
}

void ofApp::update() {
  if (kinect.isConnected()) {
    kinect.update();
    if (kinect.isFrameNew()) {
      // Write Kinect depth map pixels to the mask and fade.
      int w = kinect.width;
      int h = kinect.height;
      for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
          int i = y * w+ x;
          unsigned char k = kinect.getDepthPixels()[y * w + w - x - 1];
          unsigned char m = MAX(0, maskPixels[i] - fadeRate);
          k = ofMap(CLAMP(k, farThreshold, nearThreshold), farThreshold, nearThreshold, 0, 255);
          blurredPixels[i] = maskPixels[i] = k > m ? k : m;
        }
      }

      blurrer.blur(blurredPixels);
    }
  }
}

void ofApp::draw() {
  ofBackground(0);
  ofSetColor(255, 255, 255);
  drawImage.setFromPixels(blurredPixels, kinect.width, kinect.height, OF_IMAGE_GRAYSCALE);
  drawImage.draw(0, 0, ofGetScreenWidth(), ofGetScreenHeight());

  stringstream ss;
  ss << "Frame rate: " << ofToString(ofGetFrameRate(), 2) << endl
    //<< "Frame size: " << frameWidth << 'x' << frameHeight << endl
    //<< "Memory: " << floor(bytes / 1024 / 1024) << " MB" << endl
    << "(G) Ghost: " << (showGhost ? "on" : "off") << endl
    << "(T) Display: " << (showMask ? "mask" : "output") << endl
    << "(J/K) Mask fade rate: " << fadeRate << endl
    << "(N/B) Near threshold: " << nearThreshold << endl
    << "(F/D) Far threshold: " << farThreshold << endl
    << "([/]) Tilt angle: " << kinectAngle << endl
    << "(M) Recording: " << (recording ? "yes" : "no") << endl
    << "(R) Save Frame" << endl
    << "(ESC) Quit" << endl;
  hudFont.drawString(ss.str(), 32, 32);
  ss.str(std::string());
}

void ofApp::saveFrame() {
  drawImage.saveImage("render.png");
}

void ofApp::loadSettings() {
  ofxXmlSettings settings;
  settings.loadFile("settings.xml");
  nearThreshold = settings.getValue("settings:nearThreshold", 255);
  farThreshold = settings.getValue("settings:farThreshold", 64);
  ghostThreshold = settings.getValue("settings:ghostThreshold", 196);
  fadeRate = settings.getValue("settings:fadeRate", 3);
  kinectAngle = settings.getValue("settings:kinectAngle", 15);
  showGhost = settings.getValue("settings:showGhost", true);
  blurAmount = settings.getValue("settings:blurAmount", 3);
}

void ofApp::writeSettings() {
  ofxXmlSettings settings;
  settings.setValue("settings:nearThreshold", nearThreshold);
  settings.setValue("settings:farThreshold", farThreshold);
  settings.setValue("settings:ghostThreshold", ghostThreshold);
  settings.setValue("settings:fadeRate", fadeRate);
  settings.setValue("settings:kinectAngle", kinectAngle);
  settings.setValue("settings:showGhost", showGhost);
  settings.setValue("settings:blurAmount", blurAmount);
  settings.saveFile("settings.xml");
}

void ofApp::keyPressed(int key) {
}

void ofApp::keyReleased(int key) {
	switch (key) {
		case ' ':
			showHud = !showHud;
			break;

		case 'r':
			saveFrame();
			break;

		case 'g':
			showGhost = !showGhost;
			writeSettings();
			break;

		case 't':
			showMask = !showMask;
			break;

		case 'j':
			fadeRate--;
			if (fadeRate < 0) fadeRate = 0;
			cout << "Fade rate: " << fadeRate << endl;
			writeSettings();
			break;

		case 'k':
			fadeRate++;
			if (fadeRate > 255) fadeRate = 255;
			cout << "Fade rate: " << fadeRate << endl;
			writeSettings();
			break;

		case 'n':
			nearThreshold++;
			if (nearThreshold > 255) nearThreshold = 255;
			cout << "Near threshold: " << nearThreshold << endl;
			writeSettings();
			break;

		case 'b':
			nearThreshold--;
			if (nearThreshold <= farThreshold) nearThreshold = farThreshold + 1;
			cout << "Near threshold: " << nearThreshold << endl;
			writeSettings();
			break;

		case 'f':
			farThreshold++;
			if (farThreshold >= nearThreshold) farThreshold = nearThreshold - 1;
			cout << "Far threshold: " << farThreshold << endl;
			writeSettings();
			break;

		case 'd':
			farThreshold--;
			if (farThreshold < 0) farThreshold = 0;
			cout << "Far threshold: " << farThreshold << endl;
			writeSettings();
			break;

		case 'm':
			recording = !recording;
			if (recording) {
				// Create recording folder from timestamp.
				time_t rawtime;
				struct tm * timeinfo;
				char buffer [80];

				time (&rawtime);
				timeinfo = localtime (&rawtime);

				strftime(buffer, 80, "%Y-%m-%d %H-%M-%S", timeinfo);
				stringstream ss;
				ss << buffer;
				recordingPath = ss.str();

				ofDirectory d;
				if (!d.doesDirectoryExist(recordingPath)) {
					d.createDirectory(recordingPath);
					recordingImageIndex = 0;
				}

				cout << "Recording to " << recordingPath << endl;
			}
			else {
				cout << "Recorded " << recordingImageIndex << " frames to " << recordingPath << endl;
				recordingPath = "";
				recordingImageIndex = 0;
			}
			break;

		case '[':
			kinectAngle -= 0.5;
			kinect.setCameraTiltAngle(kinectAngle);
			cout << "Tilt angle: " << kinectAngle << endl;
			writeSettings();
			break;

		case ']':
			kinectAngle += 0.5;
			kinect.setCameraTiltAngle(kinectAngle);
			cout << "Tilt angle: " << kinectAngle << endl;
			writeSettings();
			break;
	}
}

void ofApp::mouseMoved(int x, int y ) {
}

void ofApp::mouseDragged(int x, int y, int button) {
}

void ofApp::mousePressed(int x, int y, int button) {
}

void ofApp::mouseReleased(int x, int y, int button) {
}

void ofApp::windowResized(int w, int h) {
}

void ofApp::gotMessage(ofMessage msg) {
}

void ofApp::dragEvent(ofDragInfo dragInfo) {
}
