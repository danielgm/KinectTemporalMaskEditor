#include "ofApp.h"

void ofApp::setup() {
  ofSetLogLevel(OF_LOG_VERBOSE);

  screenWidth = ofGetWindowWidth();
  screenHeight = ofGetWindowHeight();

  loadSettings();

  hudFont.loadFont("verdana.ttf", 12);

  kinect.init(false, true, false);
  kinect.open();

  ofSetFrameRate(60);
  kinect.setCameraTiltAngle(0);

  frameCount = 100;
  frameWidth = kinect.width;
  frameHeight = kinect.height;

  blurrer.init(frameWidth, frameHeight, 5);

  inputPixels = new unsigned char[frameCount * frameWidth * frameHeight * 3];
  maskPixels = new unsigned char[frameWidth * frameHeight * 1];
  blurredPixels = new unsigned char[frameWidth * frameHeight * 3];
  outputPixels = new unsigned char[frameWidth * frameHeight * 3];

  // Start the mask off black.
  for (int i = 0; i < frameWidth * frameHeight; i++) {
    maskPixels[i] = 0;
  }

  // Start the input off black.
  for (int i = 0; i < frameCount * frameWidth * frameHeight * 3; i++) {
    inputPixels[i] = 0;
  }

  inputPixelsStartIndex = -1;
}

void ofApp::update() {
  bool isFrameNew = false;
  if (kinect.isConnected()) {
    kinect.update();
    if (kinect.isFrameNew()) {
      if (++inputPixelsStartIndex >= frameCount) inputPixelsStartIndex = 0;

      // Write Kinect depth map pixels to the mask and fade.
      int w = frameWidth;
      int h = frameHeight;

      unsigned char* p = inputPixels + inputPixelsStartIndex * w * h * 3;
      memcpy(p, kinect.getPixels(), w * h * 3);

      for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
          int i = y * w + x;
          unsigned char k = kinect.getDepthPixels()[i];
          unsigned char m = MAX(0, maskPixels[i] - fadeRate);
          k = ofMap(CLAMP(k, farThreshold, nearThreshold), farThreshold, nearThreshold, 0, 255);
          blurredPixels[i] = maskPixels[i] = k > m ? k : m;
        }
      }
      isFrameNew = true;
    }
  }

  if (isFrameNew) {
    blurrer.blur(blurredPixels);

    for (int i = 0; i < frameWidth * frameHeight; i++) {
      int frameIndex = inputPixelsStartIndex + ofMap(blurredPixels[i], 0, 255, 0, frameCount - 1);
      if (frameIndex >= frameCount) frameIndex -= frameCount;

      for (int c = 0; c < 3; c++) {
        outputPixels[i * 3 + c] = MIN(255, inputPixels[frameIndex * frameWidth * frameHeight * 3 + i * 3 + c] + maskPixels[i] / 52);
      }
    }
  }
}

void ofApp::draw() {
  ofBackground(0);
  ofSetColor(255, 255, 255);
  if (showMask) {
    drawImage.setFromPixels(blurredPixels, frameWidth, frameHeight, OF_IMAGE_GRAYSCALE);
  }
  else {
    drawImage.setFromPixels(outputPixels, frameWidth, frameHeight, OF_IMAGE_COLOR);
  }
  drawImage.draw(screenWidth, 0, -screenWidth, screenHeight);

  stringstream ss;
  ss << "Frame rate: " << ofToString(ofGetFrameRate(), 2) << endl
    //<< "Frame size: " << frameWidth << 'x' << frameHeight << endl
    //<< "Memory: " << floor(bytes / 1024 / 1024) << " MB" << endl
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
  fadeRate = settings.getValue("settings:fadeRate", 3);
  kinectAngle = settings.getValue("settings:kinectAngle", 15);
  blurAmount = settings.getValue("settings:blurAmount", 3);
}

void ofApp::writeSettings() {
  ofxXmlSettings settings;
  settings.setValue("settings:nearThreshold", nearThreshold);
  settings.setValue("settings:farThreshold", farThreshold);
  settings.setValue("settings:fadeRate", fadeRate);
  settings.setValue("settings:kinectAngle", kinectAngle);
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
