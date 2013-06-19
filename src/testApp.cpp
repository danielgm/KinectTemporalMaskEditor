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
	loading = false;
	
	hudFont.loadFont("verdana.ttf", 16);
	messageFont.loadFont("verdana.ttf", 54);
	submessageFont.loadFont("verdana.ttf", 24);
	creditFont.loadFont("verdana.ttf", 16);
	
	addInputClip("Cheetahs on the Edge", "cheetah/cheetah-s", "Footage courtesy of National Geographic");
	addInputClip("Fire Tennis", "firetennis", "Footage courtesy of the Slow Mo Guys");
	addInputClip("Raptor Strikes", "raptor_retimed", "Footage courtesy of Smarter Every Day. Retimed by Darren DeCoursey.");
	addInputClip("Shuttle Ascent", "shascenthd_retimed", "Footage courtesy of NASA. Retimed by Darren DeCoursey.");
	
	nearThreshold = 0.8;
	farThreshold = 0.6;
	fadeRate = 0.005;
	
	screenWidth = ofGetScreenWidth();
	screenHeight = ofGetScreenHeight();
	
	frameCount = 0;
	frameWidth = kinect.width;
	frameHeight = kinect.height;
	calculateDrawSize();
	
	openCL.setup();
	openCL.loadProgramFromFile("KinectTemporalMaskEditor.cl");
	openCL.loadKernel("updateMask");
	openCL.loadKernel("temporalVideoMask");
	openCL.loadKernel("msa_boxblur");

	depthBuffer.initWithoutTexture(kinect.width, kinect.height, 1, CL_R, CL_UNORM_INT8);
	
	maskBuffer[0].initWithoutTexture(kinect.width, kinect.height, 1, CL_R, CL_UNORM_INT8);
	maskBuffer[1].initWithoutTexture(kinect.width, kinect.height, 1, CL_R, CL_UNORM_INT8);
	activeMaskBuffer = 0;
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
		if (movieFramesAllocated) {
			depthBuffer.write(kinect.getDepthPixels());
			
			msa::OpenCLKernel *kernel;

			kernel = openCL.kernel("updateMask");
			kernel->setArg(0, depthBuffer.getCLMem());
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
			for(int i = 0; i < 5; i++) {
				cl_int offset = i * i / 2 + 1;
				kernel->setArg(0, maskBuffer[activeMaskBuffer].getCLMem());
				kernel->setArg(1, blurredBuffer.getCLMem());
				kernel->setArg(2, offset);
				kernel->run2D(frameWidth, frameHeight);
			}
			
			openCL.finish();

			kernel = openCL.kernel("temporalVideoMask");
			kernel->setArg(0, inputBuffer.getCLMem());
			kernel->setArg(1, blurredBuffer.getCLMem());
			kernel->setArg(2, distortedBuffer.getCLMem());
			kernel->setArg(3, reverseTime);
			kernel->run2D(frameWidth, frameHeight);
			
			openCL.finish();
		}
	}
}

void testApp::draw() {
	ofBackground(0);
	ofSetColor(255, 255, 255);
	
	if (showMask) {
		drawImage.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height, OF_IMAGE_GRAYSCALE);
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
		
		str << "(0) Clear frames." << endl;
		for (int i = 0; i < MIN(9, inputClips.size()); i++) {
			str << "(" << (i + 1) << ") " << inputClips.at(i).title << endl;
		}
		hudFont.drawString(str.str(), 32, 32);
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
		hudFont.drawString(str.str(), 32, 552);
		str.str(std::string());
	}
	
	if (loading) {
		ofSetColor(255);
		string message = "Loading";
		messageFont.drawString(message,
							   (screenWidth - messageFont.stringWidth(message)) / 2,
							   (screenHeight - messageFont.stringHeight(message)) / 2);
		
		stringstream str;
		str << floor((float)loader.getFramesLoaded() / loader.getFrameCount() * 100) << '%';
		string submessage = str.str();
		submessageFont.drawString(submessage,
								  (screenWidth - submessageFont.stringWidth(submessage)) / 2,
								  (screenHeight + messageFont.stringHeight(message)) / 2 + 10);
	}
	else {
		ofSetColor(194);
		creditFont.drawString(credit, 32, screenHeight - 20);
	}
	
	ofSetColor(255, 0, 0);
	ofNoFill();
	ofCircle(480, 270, 10);
}

void testApp::exit() {
	clearMovieFrames();
}

void testApp::clearMovieFrames() {
	if (movieFramesAllocated) {
		delete[] distortedPixels;
		
		frameCount = 0;
		frameWidth = kinect.width;
		frameHeight = kinect.height;
		calculateDrawSize();
		
		movieFramesAllocated = false;
	}
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
				
				loader.load(inputClips.at(key - 1).path);
				
				frameCount = loader.getFrameCount();
				frameWidth = loader.getFrameWidth();
				frameHeight = loader.getFrameHeight();
				inputPixels = loader.getPixels();
				
				calculateDrawSize();
				
				blurredBuffer.initWithoutTexture(frameWidth, frameHeight, 1, CL_R, CL_UNORM_INT8);
				distortedBuffer.initWithoutTexture(frameWidth, frameHeight, 1, CL_RGBA, CL_UNORM_INT8);
				distortedPixels = new unsigned char[frameWidth * frameHeight * 4];
				
				credit = inputClips.at(key - 1).credit;
				loading = true;
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
	cout << "gotMessage(" + msg.message + ")" << endl;
	if (msg.message == "loaded") {
		cout << "Alloc: " << frameCount * frameWidth * frameHeight * 4 << " " << openCL.info.maxMemAllocSize << endl;
		if (frameCount * frameWidth * frameHeight * 4 > openCL.info.maxMemAllocSize) {
			cout << "Too many frames!" << endl;
		}
		
		inputBuffer.initWithoutTexture(frameWidth, frameHeight, frameCount, CL_RGBA, CL_UNORM_INT8);
		inputBuffer.write(inputPixels, true);
		movieFramesAllocated = true;
		loading = false;
	}
}

void testApp::dragEvent(ofDragInfo dragInfo) {
}

