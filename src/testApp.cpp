#include "testApp.h"

using namespace ofxCv;
using namespace cv;

void testApp::setup() {
	ofSetLogLevel(OF_LOG_VERBOSE);
	
	// enable depth->video image calibration
	kinect.setRegistration(true);
    
	kinect.init(false, false);
	kinect.open();
	
	ofSetFrameRate(60);
	kinect.setCameraTiltAngle(0);
	
	gotKinectFrame = false;
	
	readFrames("out12.mov");
	
	showMask = true;
}

void testApp::update() {
	kinect.update();
	if (kinect.isFrameNew()) {
		maskOfp.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height, OF_IMAGE_GRAYSCALE);
		maskOfp.resize(frameWidth, frameHeight);
		tempPixels = maskOfp.getPixels();
		
		for (int i = 0; i < frameWidth * frameHeight; i++) {
			// Threshold at 128, then normalize.
			int tempPixel = (max(128, (int)tempPixels[i]) - 128) * 255 * 2;
			
			// Take the brighter pixel and fade back to black slowly.
			maskPixelsDetail[i] = tempPixel > maskPixelsDetail[i] ? tempPixel : max(0, maskPixelsDetail[i] - 128);
			
			// Copy low-fi value to maskPixels[].
			maskPixels[i] = maskPixelsDetail[i] / 255;
		}
		
		maskOfp.setFromPixels(maskPixels, frameWidth, frameHeight, OF_IMAGE_GRAYSCALE);
		blur(maskOfp, maskOfp, 50);
		tempPixels = maskOfp.getPixels();
		
		// Horizontal flip. No method for this in ofxCv?
		for (int x = 0; x < frameWidth; x++) {
			for (int y = 0; y < frameHeight; y++) {
				maskPixels[y * frameWidth + x] = tempPixels[y * frameWidth + (frameWidth - x - 1)];
			}
		}
		
		gotKinectFrame = true;
	}
}

void testApp::draw() {
	ofBackground(0);
	
	if (frameWidth > 0) {
		ofSetColor(255, 255, 255);
		
		if (gotKinectFrame) {
			for (int x = 0; x < frameWidth; x++) {
				for (int y = 0; y < frameHeight; y++) {
					int frameIndex = maskPixels[y * frameWidth + x] * frameCount / 255;
					frameIndex = max(0, min(frameCount-1, frameIndex));
					unsigned char* frame = frames[frameIndex];
					
					for (int c = 0; c < 3; c++) {
						int pixelIndex = y * frameWidth * 3 + x * 3 + c;
						distortedPixels[pixelIndex] = frame[pixelIndex];
					}
				}
			 }
			
			gotKinectFrame = false;
		}
		
		if (showMask) {
			mask.setFromPixels(maskPixels, frameWidth, frameHeight, OF_IMAGE_GRAYSCALE);
			mask.draw(0, 0);
		}
		else {
			distorted.setFromPixels(distortedPixels, frameWidth, frameHeight, OF_IMAGE_COLOR);
			distorted.draw(0, 0);
		}
	}
}

void testApp::exit() {
	clearFrames();
}

void testApp::readFrames(string filename) {
	player.loadMovie(filename);
	player.play();
	player.setPaused(true);
	
	frameCount = 20; //player.getTotalNumFrames();
	frameWidth = player.width;
	frameHeight = player.height;
	
	distortedPixels = new unsigned char[frameWidth * frameHeight * 3];
	
	maskPixels = new unsigned char[frameWidth * frameHeight];
	maskPixelsDetail = new unsigned short int[frameWidth * frameHeight];
	for (int p = 0; p < frameWidth * frameHeight; p++) {
		maskPixels[p] = maskPixelsDetail[p] = 0;
	}
	
	cout << "Loading frames..." << endl;
	
	unsigned char* copyPixels;
	for (int i = 0; i < frameCount; i++) {
		cout << i << " of " << frameCount << endl;
		copyPixels = player.getPixels();
		unsigned char* framePixels = new unsigned char[frameWidth * frameHeight * 3];
		for (int i = 0; i < frameWidth * frameHeight * 3; i++) {
			framePixels[i] = copyPixels[i];
		}
		frames.push_back(framePixels);
		player.nextFrame();
	}
	
	player.closeMovie();
	
	distorted.setFromPixels(frames[0], frameWidth, frameHeight, OF_IMAGE_COLOR);
}

void testApp::clearFrames() {
	for (int i = 0; i < frameCount; i++) {
		delete[] frames[i];
	}
	
	delete[] distortedPixels;
	
	delete[] maskPixels;
	delete[] maskPixelsDetail;
		
	frameCount = 0;
	frameWidth = 0;
	frameHeight = 0;
}

void testApp::writeDistorted() {
	distorted.setFromPixels(distortedPixels, frameWidth, frameHeight, OF_IMAGE_COLOR);
	distorted.saveImage("distorted.tga", OF_IMAGE_QUALITY_BEST);
}

void testApp::keyPressed(int key) {
}

void testApp::keyReleased(int key) {
	switch (key) {
		case 's':
			writeDistorted();
			break;
			
		case 'x':
			clearFrames();
			break;
		
		case 't':
			showMask = !showMask;
			break;
	}
}

void testApp::mouseMoved(int x, int y) {
}

void testApp::mouseDragged(int x, int y, int button) {
}

void testApp::mousePressed(int x, int y, int button) {
}

void testApp::mouseReleased(int x, int y, int button) {}

void testApp::windowResized(int w, int h) {
}

void testApp::gotMessage(ofMessage msg) {
}
void testApp::dragEvent(ofDragInfo dragInfo) {
	for (int i = 0; i < dragInfo.files.size(); i++) {
		string filename = dragInfo.files[i];
		vector<string> tokens = ofSplitString(filename, ".");
		string extension = tokens[tokens.size() - 1];
		if (extension == "mov") {
			clearFrames();
			readFrames(filename);
		}
		else {
			cout << "Unknown file type: " << extension << endl;
		}
	}
}

