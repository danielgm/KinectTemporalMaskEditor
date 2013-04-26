#include "testApp.h"

using namespace ofxCv;
using namespace cv;

void testApp::setup() {
	ofSetLogLevel(OF_LOG_VERBOSE);
	
	readFolderFrames("snowboard-s");
    
	kinect.init(false, false);
	kinect.open();
	
	ofSetFrameRate(60);
	kinect.setCameraTiltAngle(0);
	
	maskInitialized = false;
	gotKinectFrame = false;
	showMask = true;
}

void testApp::initMask() {
	maskOfp.allocate(kinect.width, kinect.height, OF_IMAGE_GRAYSCALE);
	maskPixels = maskOfp.getPixels();
	
	maskPixelsDetail = new unsigned short int[kinect.width * kinect.height];
	for (int i = 0; i < kinect.width * kinect.height; i++) {
		maskPixelsDetail[i] = 0;
	}
}

void testApp::update() {
	kinect.update();
	if (kinect.isFrameNew()) {
		if (!maskOfp.isAllocated()) {
			initMask();
		}
		
		kinectPixels = kinect.getDepthPixels();
		for (int x = 0; x < kinect.width; x++) {
			for (int y = 0; y < kinect.height; y++) {
				int i = y * kinect.width + x;
				
				// Normalize between 128 and 192. Multiply by another 255 for the higher-res
				// maskPixelsDetail[]. Horizontal flip. No method for this in ofxCv?
				int kinectPixel = (max(128, min(192, (int)kinectPixels[y * kinect.width + (kinect.width - x - 1)])) - 128) * 255 / (192 - 128) * 255;
				
				// Take the brighter pixel and fade back to black slowly.
				maskPixelsDetail[i] = kinectPixel > maskPixelsDetail[i] ? kinectPixel : max(0, maskPixelsDetail[i] - 128);
				
				// Copy low-fi value to maskPixels[].
				maskPixels[i] = maskPixelsDetail[i] / 255;
			}
		}
		
		maskOfp.setFromPixels(maskPixels, kinect.width, kinect.height, OF_IMAGE_GRAYSCALE);
		if (frameWidth >= 1080) {
			// HACK: Resizing straight to 1080p crashes.
			maskOfp.resize(frameWidth/2, frameHeight/2, OF_INTERPOLATE_NEAREST_NEIGHBOR);
		}
		maskOfp.resize(frameWidth, frameHeight, OF_INTERPOLATE_NEAREST_NEIGHBOR);
		blur(maskOfp, maskOfp, 50);
		maskPixels = maskOfp.getPixels();

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
			mask.setFromPixels(maskOfp);
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

void testApp::readMovieFrames(string filename) {
	player.loadMovie(filename);
	player.play();
	player.setPaused(true);
	
	frameCount = player.getTotalNumFrames();
	frameWidth = player.width;
	frameHeight = player.height;
	
	cout << "Image size: " << frameWidth << "x" << frameHeight << endl;
	
	distortedPixels = new unsigned char[frameWidth * frameHeight * 3];
	
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

void testApp::readFolderFrames(string folder) {
	ofFile file;
	bool foundFile = false;
	ofImage image;
	unsigned char* copyPixels;
	int index, i, count;
	string indexString;
	
	frameCount = 0;
	frameWidth = 0;
	frameHeight = 0;
	
	for (index = 0; index < 100; index++) {
		indexString = ofToString(index);
		while (indexString.length() < 4) indexString = "0" + indexString;
		if (file.doesFileExist(folder + "/frame" + indexString + ".jpg")) {
			foundFile = true;
			break;
		}
	}
	
	if (foundFile) {
		cout << "Loading frames... ";
		
		count = 0;
		while (file.doesFileExist(folder + "/frame" + indexString + ".jpg")) {
			image.loadImage(folder + "/frame" + indexString + ".jpg");
			if (count == 0) {
				frameWidth = image.width;
				frameHeight = image.height;
			}
			
			copyPixels = image.getPixels();
			unsigned char* framePixels = new unsigned char[frameWidth * frameHeight * 3];
			for (i = 0; i < frameWidth * frameHeight * 3; i++) {
				framePixels[i] = copyPixels[i];
			}
			frames.push_back(framePixels);
			
			count++;
			index++;
			indexString = ofToString(index);
			while (indexString.length() < 4) indexString = "0" + indexString;
		}
		
		cout << "done." << endl;
		
		frameCount = count;
		
		distortedPixels = new unsigned char[frameWidth * frameHeight * 3];
	}
	else {
		cout << "Failed to find frames in path: " + folder + "/frame%4d.jpg" << endl;
	}
}

void testApp::clearFrames() {
	for (int i = 0; i < frameCount; i++) {
		delete[] frames[i];
	}
	
	delete[] distortedPixels;
	
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
			readMovieFrames(filename);
		}
		else {
			cout << "Unknown file type: " << extension << endl;
		}
	}
}

