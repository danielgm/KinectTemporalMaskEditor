#include "testApp.h"

using namespace ofxCv;
using namespace cv;

void testApp::setup() {
	ofSetLogLevel(OF_LOG_VERBOSE);
	
	readFolderFrames("cheetah");
    
	kinect.init(false, false);
	kinect.open();
	
	ofSetFrameRate(60);
	kinect.setCameraTiltAngle(0);
	
	gotKinectFrame = false;
	
	showMask = true;
}

void testApp::update() {
	kinect.update();
	if (kinect.isFrameNew()) {
		maskOfp.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height, OF_IMAGE_GRAYSCALE);
		maskOfp.resize(frameWidth/2, frameHeight/2, OF_INTERPOLATE_NEAREST_NEIGHBOR); // Resizing straight to frame dimensions crashes.
		maskOfp.resize(frameWidth, frameHeight, OF_INTERPOLATE_NEAREST_NEIGHBOR);
		kinectPixels = maskOfp.getPixels();
		
		for (int i = 0; i < frameWidth * frameHeight; i++) {
				
			// Normalize between 128 and 192. Multiply by another 255 for the higher-res
			// maskPixelsDetail[]. 
			int kinectPixel = (int)kinectPixels[i] * 255; //(max(128, min(192, (int)kinectPixels[i])) - 128) * 255 / (192 - 128) * 255;
			
			// enable depth->video image calibration
			kinect.setRegistration(true);
			
			// Take the brighter pixel and fade back to black slowly.
			maskPixelsDetail[i] = kinectPixel > maskPixelsDetail[i] ? kinectPixel : max(0, maskPixelsDetail[i] - 255);
			
			// Copy low-fi value to maskPixels[].
			maskPixels[i] = maskPixelsDetail[i] / 255;
		}
		
		maskOfp.setFromPixels(maskPixels, frameWidth, frameHeight, OF_IMAGE_GRAYSCALE);
		blur(maskOfp, maskOfp, 50);
		kinectPixels = maskOfp.getPixels();

		// Horizontal flip. No method for this in ofxCv?
		for (int x = 0; x < frameWidth; x++) {
			for (int y = 0; y < frameHeight; y++) {
				maskPixels[y * frameWidth + x] = kinectPixels[y * frameWidth + (frameWidth - x - 1)];
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

void testApp::readMovieFrames(string filename) {
	player.loadMovie(filename);
	player.play();
	player.setPaused(true);
	
	frameCount = player.getTotalNumFrames();
	frameWidth = player.width;
	frameHeight = player.height;
	
	cout << "Image size: " << frameWidth << "x" << frameHeight << endl;
	
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
		while (count < 50 && file.doesFileExist(folder + "/frame" + indexString + ".jpg")) {
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
		
		maskPixels = new unsigned char[frameWidth * frameHeight];
		maskPixelsDetail = new unsigned short int[frameWidth * frameHeight];
		for (i = 0; i < frameWidth * frameHeight; i++) {
			maskPixels[i] = maskPixelsDetail[i] = 0;
		}
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
			readMovieFrames(filename);
		}
		else {
			cout << "Unknown file type: " << extension << endl;
		}
	}
}

