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
	
	showHud = true;
	showMask = false;
	showGhost = true;
	reverseTime = true;
	recording = false;
	
	hudFont.loadFont("verdana.ttf", 16);
	messageFont.loadFont("verdana.ttf", 54);
	submessageFont.loadFont("verdana.ttf", 24);
	
	nearThreshold = 0.8;
	farThreshold = 0.6;
	fadeRate = 0.005;
	
	screenWidth = ofGetWindowWidth();
	screenHeight = ofGetWindowHeight();
	
	frameOffset = 0;
	
	openCL.setup();
	openCL.loadProgramFromFile("VideoLoops.cl");
	openCL.loadKernel("updateMask");
	openCL.loadKernel("temporalVideoMask0");
	openCL.loadKernel("temporalVideoMask1");
	openCL.loadKernel("temporalVideoMask2");
	openCL.loadKernel("temporalVideoMask3");
	openCL.loadKernel("msa_boxblur");
	
	blurAmount = 3;
	
	frameCount = 0;
	ofFile file;
	while (file.doesFileExist("layer0/frame" + ofToString(frameCount + 1, 0, 4, '0') + ".png")) {
		frameCount++;
	}
	
	if (frameCount * NUM_LAYERS > openCL.info.maxReadImageArgs) {
		cerr << "Too many frames! maxReadImageArgs=" << openCL.info.maxReadImageArgs << endl;
		return;
	}
	
	if (frameCount <= 0) {
		cerr << "Frames not found" << endl;
		cerr << "layer0/frame" + ofToString(frameCount + 1, 0, 4, '0') + ".png" << endl;
		return;
	}
	
	// Determine image size from the first frame.
	ofImage image;
	image.loadImage("layer0/frame0001.png");
	frameWidth = image.width;
	frameHeight = image.height;
	calculateDrawSize();
	
	cout << "Alloc: " << frameCount * frameWidth * frameHeight * 4 << " " << openCL.info.maxMemAllocSize << endl;
	if (frameCount * frameWidth * frameHeight * 4 > openCL.info.maxMemAllocSize) {
		cerr << "Too many pixels/frames! frameCount * w * h * 4 bytes must be less than maxMemAllocSize=" << openCL.info.maxMemAllocSize << endl;
		return;
	}
	
	blurredBuffer.initWithoutTexture(frameWidth, frameHeight, 1, CL_A, CL_UNORM_INT8);
	distortedBuffer.initWithoutTexture(frameWidth, frameHeight, 1, CL_RGBA, CL_UNORM_INT8);
	blurredPixels = new unsigned char[frameWidth * frameHeight * 1];
	distortedPixels = new unsigned char[frameWidth * frameHeight * 4];
	
	kinectDepthBuffer.initWithoutTexture(kinect.width, kinect.height, 1, CL_A, CL_UNORM_INT8);
	maskBuffer[0].initWithoutTexture(kinect.width, kinect.height, 1, CL_A, CL_UNORM_INT8);
	maskBuffer[1].initWithoutTexture(kinect.width, kinect.height, 1, CL_A, CL_UNORM_INT8);
	activeMaskBuffer = 0;
	
	cout << "Loading " << frameCount << " frames at " << frameWidth << "x" << frameHeight << "... ";
	
	unsigned char* inputPixels = new unsigned char[frameCount * frameWidth * frameHeight * 4];
	for (int layer = 0; layer < NUM_LAYERS; layer++) {
		inputBuffer[layer].initWithoutTexture(frameWidth, frameHeight, frameCount, CL_RGBA, CL_UNORM_INT8);
		for (int frameIndex = 0; frameIndex < frameCount; frameIndex++) {
			image.loadImage("layer" + ofToString(layer) + "/frame" + ofToString(frameIndex + 1, 0, 4, '0') + ".png");
			
			unsigned char* copyPixels = image.getPixels();
			for (int i = 0; i < frameWidth * frameHeight; i++) {
				inputPixels[frameIndex * frameWidth * frameHeight * 4 + i * 4 + 0] = copyPixels[i * 3 + 0];
				inputPixels[frameIndex * frameWidth * frameHeight * 4 + i * 4 + 1] = copyPixels[i * 3 + 1];
				inputPixels[frameIndex * frameWidth * frameHeight * 4 + i * 4 + 2] = copyPixels[i * 3 + 2];
				inputPixels[frameIndex * frameWidth * frameHeight * 4 + i * 4 + 3] = 255;
			}
		}
		inputBuffer[layer].write(inputPixels, true);
	}
	delete[] inputPixels;
	
	movieFramesAllocated = true;
	previousTime = ofGetSystemTime();
}

void testApp::update() {
	if (frameCount > 0) {
		// Increment the frame offset at the given FPS.
		long now = ofGetSystemTime();
		int t = now - previousTime;
		int d = floor(t / 1000.0 * frameOffsetFps);
		frameOffset += d;
		while (frameOffset > frameCount) frameOffset -= frameCount;
		previousTime = now + floor(d * 1000.0 / frameOffsetFps) - t;
	}
	
	kinect.update();
	if (kinect.isFrameNew() && movieFramesAllocated) {
		kinectDepthBuffer.write(kinect.getDepthPixels());
		
		msa::OpenCLKernel *kernel;
		
		kernel = openCL.kernel("updateMask");
		kernel->setArg(0, kinectDepthBuffer.getCLMem());
		kernel->setArg(1, maskBuffer[activeMaskBuffer].getCLMem());
		kernel->setArg(2, maskBuffer[1 - activeMaskBuffer].getCLMem());
		kernel->setArg(3, nearThreshold);
		kernel->setArg(4, farThreshold);
		kernel->setArg(5, fadeRate);
		kernel->run2D(kinect.width, kinect.height);
		
		activeMaskBuffer = 1 - activeMaskBuffer;
		openCL.finish();
		
		// Blur and resize. Might be better to separate these.
		kernel = openCL.kernel("msa_boxblur");
		for(int i = 0; i < blurAmount; i++) {
			cl_int offset = i * i / 2 + 1;
			kernel->setArg(0, maskBuffer[activeMaskBuffer].getCLMem());
			kernel->setArg(1, maskBuffer[1 - activeMaskBuffer].getCLMem());
			kernel->setArg(2, offset);
			kernel->run2D(frameWidth, frameHeight);
			
			activeMaskBuffer = 1 - activeMaskBuffer;
			openCL.finish();
		}
		
		// One last blur into the blurred buffer.
		cl_int offset = blurAmount * blurAmount / 2 + 1;
		kernel->setArg(0, maskBuffer[activeMaskBuffer].getCLMem());
		kernel->setArg(1, blurredBuffer.getCLMem());
		kernel->setArg(2, offset);
		kernel->run2D(frameWidth, frameHeight);
		
		openCL.finish();
		
		float normalizedFrameOffset = float(frameOffset) / frameCount;
		
		/// DEBUG: Getting rid of frame offset for now.
		normalizedFrameOffset = 0;
		
		for (int layer = 0; layer < NUM_LAYERS; layer++) {
			kernel = openCL.kernel("temporalVideoMask" + ofToString(layer));
			kernel->setArg(0, inputBuffer[layer].getCLMem());
			kernel->setArg(1, blurredBuffer.getCLMem());
			kernel->setArg(2, distortedBuffer.getCLMem());
			kernel->setArg(3, normalizedFrameOffset);
			kernel->run2D(frameWidth, frameHeight);
			
			openCL.finish();
		}
		
		openCL.finish();
	}
}

void testApp::draw() {
	ofBackground(0);
	ofSetColor(255, 255, 255);
	
	if (showMask) {
		blurredBuffer.read(blurredPixels, true);
		drawImage.setFromPixels(blurredPixels, frameWidth, frameHeight, OF_IMAGE_GRAYSCALE);
		drawImage.draw((screenWidth - drawWidth)/2, (screenHeight - drawHeight)/2, drawWidth, drawHeight);
	}
	if (movieFramesAllocated) {
		if (!showMask) {
			distortedBuffer.read(distortedPixels, true);
			drawImage.setFromPixels(distortedPixels, frameWidth, frameHeight, OF_IMAGE_COLOR_ALPHA);
			drawImage.draw((screenWidth - drawWidth)/2, (screenHeight - drawHeight)/2, drawWidth, drawHeight);
		}
		
		if (recording) {
			string filenameIndexStr = ofToString(recordingImageIndex++);
			while (filenameIndexStr.size() < 4) filenameIndexStr = "0" + filenameIndexStr;
			drawImage.saveImage(recordingPath + "/frame" + filenameIndexStr + ".jpg");
		}
	}
	
	if (showHud) {
		stringstream str;
		
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
		hudFont.drawString(str.str(), 32, 552);
		str.str(std::string());
	}
	
	ofSetColor(255, 0, 0);
	ofNoFill();
	ofCircle(480, 270, 10);
}

void testApp::exit() {
	delete[] distortedPixels;
	delete[] blurredPixels;
}

void testApp::writeDistorted() {
	drawImage.setFromPixels(distortedPixels, frameWidth, frameHeight, OF_IMAGE_COLOR);
	drawImage.saveImage("distorted.tga", OF_IMAGE_QUALITY_BEST);
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

void testApp::gotMessage(ofMessage msg) {}

void testApp::dragEvent(ofDragInfo dragInfo) {
}

