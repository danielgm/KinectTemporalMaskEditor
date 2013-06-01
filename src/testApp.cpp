#include "testApp.h"

using namespace ofxCv;
using namespace cv;

void testApp::setup() {
	ofSetLogLevel(OF_LOG_VERBOSE);
    
	kinect.init(false, false);
	kinect.open();
	
	kinectAngle = 0;
	ofSetFrameRate(60);
	kinect.setCameraTiltAngle(kinectAngle);
	
	movieFramesAllocated = false;
	maskInitialized = false;
	
	showHud = true;
	showMask = false;
	showGhost = true;
	reverseTime = true;
	recording = false;
	
	font.loadFont("verdana.ttf", 16);
	
	addInputClip("Cheetahs on the Edge", "cheetah/cheetah-s", "Footage courtesy of National Geographic");
	addInputClip("Fire Tennis", "firetennis", "Footage courtesy of the Slow Mo Guys");
	addInputClip("Raptor Strikes", "raptor_retimed", "Footage courtesy of Smarter Every Day");
	
	nearThreshold = 214;
	farThreshold = 164;
	fadeRate = 256;
	
	screenWidth = ofGetScreenWidth();
	screenHeight = ofGetScreenHeight();
	
	distorted.allocate(screenWidth, screenHeight, OF_IMAGE_COLOR);
	
	calculateDrawSize();
	
	ofBackground(0);
}

void testApp::initMask() {
	maskOfp.allocate(kinect.width, kinect.height, OF_IMAGE_GRAYSCALE);
	maskPixels = maskOfp.getPixels();
	
	maskPixelsDetail = new unsigned short int[kinect.width * kinect.height];
	for (int i = 0; i < kinect.width * kinect.height; i++) {
		maskPixelsDetail[i] = 0;
	}
}

inputClip testApp::addInputClip(string title, string path, string credit) {
	inputClip clip;
	clip.title = title;
	clip.path = path;
	clip.credit = credit;
	inputClips.push_back(clip);
	return clip;
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
				
				int kinectPixel = kinectPixels[y * kinect.width + x];
				// Normalize between near and far thresholds.
				kinectPixel = (max(farThreshold, min(nearThreshold, kinectPixel)) - farThreshold) * 255 / (nearThreshold - farThreshold);
				
				// Set the pixel in the recent mask.
				kinectPixels[i] = kinectPixel;
				
				// Multiply by another 255 for the higher-res maskPixelsDetail[].
				kinectPixel *= 255;
				
				// Take the brighter pixel and fade back to black slowly.
				maskPixelsDetail[i] = kinectPixel > maskPixelsDetail[i] ? kinectPixel : max(0, maskPixelsDetail[i] - fadeRate);
				
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
		
		if (showGhost) {
			ghostOfp.setFromPixels(kinectPixels, kinect.width, kinect.height, OF_IMAGE_GRAYSCALE);
			if (frameWidth >= 1080) {
				// HACK: Resizing straight to 1080p crashes.
				ghostOfp.resize(frameWidth/2, frameHeight/2, OF_INTERPOLATE_NEAREST_NEIGHBOR);
			}
			ghostOfp.resize(frameWidth, frameHeight, OF_INTERPOLATE_NEAREST_NEIGHBOR);
			ghostPixels = ghostOfp.getPixels();
		}
		
		for (int x = 0; x < frameWidth; x++) {
			for (int y = 0; y < frameHeight; y++) {
				// Horizontal flip.
				int frameIndex = maskPixels[y * frameWidth + (frameWidth - x - 1)] * frameCount / 255;
				frameIndex = max(0, min(frameCount-1, frameIndex));
				if (!reverseTime) frameIndex = frameCount - frameIndex - 1;
				
				for (int c = 0; c < 3; c++) {
					int pixelIndex = y * frameWidth * 3 + x * 3 + c;
					distortedPixels[pixelIndex] = inputPixels[frameIndex * frameWidth * frameHeight * 3 + pixelIndex];
					if (showGhost) {
						distortedPixels[pixelIndex] = MIN(255, distortedPixels[pixelIndex] + 20 * ghostPixels[y * frameWidth + (frameWidth - x - 1)] / 255);
					}
				}
			}
		}
	}
}

void testApp::draw() {
	ofBackground(0);
	ofSetColor(255, 255, 255);
	
	if (movieFramesAllocated) {
		if (showMask) {
			mask.setFromPixels(maskPixels, frameWidth, frameHeight, OF_IMAGE_GRAYSCALE);
			mask.draw((screenWidth - drawWidth)/2, (screenHeight - drawHeight)/2, drawWidth, drawHeight);
		}
		else {
			distorted.setFromPixels(distortedPixels, frameWidth, frameHeight, OF_IMAGE_COLOR);
			distorted.draw((screenWidth - drawWidth)/2, (screenHeight - drawHeight)/2, drawWidth, drawHeight);
		}
		
		if (recording) {
			string filenameIndexStr = ofToString(recordingImageIndex++);
			while (filenameIndexStr.size() < 4) filenameIndexStr = "0" + filenameIndexStr;
			distorted.saveImage(recordingPath + "/frame" + filenameIndexStr + ".jpg");
		}
	}
	
	if (showHud) {
		stringstream str;
		
		str << "(0) Clear frames." << endl;
		for (int i = 0; i < MIN(9, inputClips.size()); i++) {
			str << "(" << (i + 1) << ") " << inputClips.at(i).title << endl;
		}
		font.drawString(str.str(), 32, 32);
		str.str(std::string());
		
		str << "Frame rate: " << ofToString(ofGetFrameRate(), 2) << endl
			<< "Frame size: " << frameWidth << 'x' << frameHeight << endl
			<< "Frame count: " << frameCount << endl
			<< "(R) Time direction: " << (reverseTime ? "reverse" : "forward") << endl
			<< "(G) Ghost: " << (showGhost ? "on" : "off") << endl
			<< "(T) Display: " << (showMask ? "mask" : "output") << endl
			<< "(J/K) Fade rate: " << fadeRate << endl
			<< "([/]) Tilt angle: " << kinectAngle << endl
			<< "(M) Recording: " << (recording ? "yes" : "no") << endl
			<< "(ESC) Quit" << endl;
		font.drawString(str.str(), 32, 552);
		str.str(std::string());
	}
	
	ofSetColor(194);
	font.drawString(credit, 32, screenHeight - 20);
}

void testApp::exit() {
	clearMovieFrames();
}

void testApp::readFolderFrames(string folder) {
	ofFile file;
	ofImage image;
	unsigned char* copyPixels;
	
	frameCount = 0;
	frameWidth = 0;
	frameHeight = 0;
	
	char indexString[5];
	sprintf(indexString, "%04d", frameCount + 1);
	while (file.doesFileExist(folder + "/frame" + indexString + ".jpg")) {
		frameCount++;
		sprintf(indexString, "%04d", frameCount + 1);
	}
	
	if (frameCount <= 0) return;
	
	// Determine image size from the first frame.
	image.loadImage(folder + "/frame0001.jpg");
	frameWidth = image.width;
	frameHeight = image.height;
	calculateDrawSize();
	
	if (frameWidth <= 0 || frameHeight <= 0) return;
	
	cout << "Loading " << frameCount << " frames at " << frameWidth << "x" << frameHeight << "... ";
	
	inputPixels = new unsigned char[frameCount * frameWidth * frameHeight * 3];
	for (int i = 0; i < frameCount; i++) {
		sprintf(indexString, "%04d", i + 1);
		image.loadImage(folder + "/frame" + indexString + ".jpg");
		
		copyPixels = image.getPixels();
		for (int j = 0; j < frameWidth * frameHeight * 3; j++) {
			unsigned char c = copyPixels[j];
			inputPixels[i * frameWidth * frameHeight * 3 + j] = c;
		}
	}
	
	cout << "done." << endl;
	
	distortedPixels = new unsigned char[frameWidth * frameHeight * 3];
	movieFramesAllocated = true;
}

void testApp::clearMovieFrames() {
	if (movieFramesAllocated) {
		if (maskOfp.isAllocated()) {
			maskOfp.clear();
			delete[] maskPixelsDetail;
		}
		
		delete[] inputPixels;
		delete[] distortedPixels;
		
		frameCount = 0;
		frameWidth = 0;
		frameHeight = 0;
		
		movieFramesAllocated = false;
	}
}

void testApp::writeDistorted() {
	distorted.setFromPixels(distortedPixels, frameWidth, frameHeight, OF_IMAGE_COLOR);
	distorted.saveImage("distorted.tga", OF_IMAGE_QUALITY_BEST);
}

void testApp::calculateDrawSize() {
	float frameAspect = (float) frameWidth / frameHeight;
	float screenAspect = (float) screenWidth / screenHeight;
	if (frameAspect < screenAspect) {
		drawHeight = screenHeight;
		drawWidth = screenHeight * frameAspect;
	}
	else {
		drawWidth = screenWidth;
		drawHeight = screenWidth / frameAspect;
	}
}

void testApp::keyPressed(int key) {
}

void testApp::keyReleased(int key) {
	switch (key) {
		case ' ':
			showHud = !showHud;
			break;
			
		case 'r':
			reverseTime = !reverseTime;
			break;
			
		case 'w':
			writeDistorted();
			break;
			
		case 'g':
			showGhost = !showGhost;
			break;
			
		case 't':
			showMask = !showMask;
			break;
			
		case 'j':
			fadeRate -= 64;
			if (fadeRate < 0) fadeRate = 0;
			cout << "Fade rate: " << fadeRate << endl;
			break;
			
		case 'k':
			fadeRate += 64;
			if (fadeRate > 255 * 255) fadeRate = 255 * 255;
			cout << "Fade rate: " << fadeRate << endl;
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
			break;
			
		case ']':
			kinectAngle += 0.5;
			kinect.setCameraTiltAngle(kinectAngle);
			cout << "Tilt angle: " << kinectAngle << endl;
			break;
		
		case '0':
			clearMovieFrames();
			break;
		
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			key -= 48;
			if (key - 1 < inputClips.size()) {
				clearMovieFrames();
				readFolderFrames(inputClips.at(key - 1).path);
				credit = inputClips.at(key - 1).credit;
			}
			break;
	}
}

void testApp::mouseMoved(int x, int y) {
}

void testApp::mouseDragged(int x, int y, int button) {
}

void testApp::mousePressed(int x, int y, int button) {
}

void testApp::mouseReleased(int x, int y, int button) {
}

void testApp::windowResized(int w, int h) {
}

void testApp::gotMessage(ofMessage msg) {
}

void testApp::dragEvent(ofDragInfo dragInfo) {
}

