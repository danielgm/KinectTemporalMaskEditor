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
	
	nearThreshold = 204;
	farThreshold = 153;
	fadeRate = 1;
	
	screenWidth = ofGetWindowWidth();
	screenHeight = ofGetWindowHeight();
	
	frameOffset = 0;
	
	blurAmount = 3;
	
	string path = "layer0";
	countFrames(path);
	calculateDrawSize(path);
	
	maskPixels = new unsigned char[kinect.width * kinect.height * 1];
	blurredPixels = new unsigned char[frameWidth * frameHeight * 4];
	inputPixels = new unsigned char[frameCount * frameWidth * frameHeight * 4];
	outputPixels = new unsigned char[frameWidth * frameHeight * 4];
	
	// Start the mask off black.
	for (int i = 0; i < kinect.width * kinect.height; i++) {
		maskPixels[i] = 0;
	}
	
	loadFrames(path, inputPixels);
	
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
		// Write Kinect depth map pixels to the mask and fade.
		unsigned char* kinectPixels = kinect.getDepthPixels();
		for (int i = 0; i < kinect.width * kinect.height; i++) {
			unsigned char k = kinectPixels[i];
			unsigned char m = MAX(0, maskPixels[i] - fadeRate);
			k = ofMap(CLAMP(k, farThreshold, nearThreshold), farThreshold, nearThreshold, 0, 255);
			blurredPixels[i] = maskPixels[i] = k > m ? k : m;
		}
		
		// FIXME: Should resize at this point. Currently just using kinect dimensions for frame size.
		assert(kinect.width == frameWidth);
		assert(kinect.height == frameHeight);
		fastBlur(blurredPixels, frameWidth, frameHeight, 5);
		
		for (int i = 0; i < frameWidth * frameHeight; i++) {
			int frameIndex = ofMap(blurredPixels[i], 0, 255, 0, frameCount - 1) + frameOffset;
			if (frameIndex >= frameCount) frameIndex -= frameCount;
			for (int c = 0; c < 4; c++) {
				outputPixels[i * 4 + c] = inputPixels[frameIndex * frameWidth * frameHeight * 4 + i * 4 + c];
			}
		}
	}
}

void testApp::draw() {
	ofBackground(0);
	ofSetColor(255, 255, 255);
	
	if (showMask) {
		drawImage.setFromPixels(blurredPixels, frameWidth, frameHeight, OF_IMAGE_GRAYSCALE);
		drawImage.draw((screenWidth - drawWidth)/2, (screenHeight - drawHeight)/2, drawWidth, drawHeight);
	}
	if (movieFramesAllocated) {
		if (!showMask) {
			drawImage.setFromPixels(outputPixels, frameWidth, frameHeight, OF_IMAGE_COLOR_ALPHA);
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
	delete[] maskPixels;
	delete[] blurredPixels;
	delete[] inputPixels;
	delete[] outputPixels;
}

void testApp::countFrames(string path) {
	frameCount = 0;
	ofFile file;
	while (file.doesFileExist(path + "/frame" + ofToString(frameCount + 1, 0, 4, '0') + ".png")) {
		frameCount++;
	}
}

void testApp::loadFrames(string path, unsigned char* pixels) {
	cout << "Loading " << frameCount << " frames at " << frameWidth << "x" << frameHeight << "... ";
	
	ofImage image;
	for (int frameIndex = 0; frameIndex < frameCount; frameIndex++) {
		image.loadImage(path + "/frame" + ofToString(frameIndex + 1, 0, 4, '0') + ".png");
		
		unsigned char* copyPixels = image.getPixels();
		for (int i = 0; i < frameWidth * frameHeight; i++) {
			pixels[frameIndex * frameWidth * frameHeight * 4 + i * 4 + 0] = copyPixels[i * 3 + 0];
			pixels[frameIndex * frameWidth * frameHeight * 4 + i * 4 + 1] = copyPixels[i * 3 + 1];
			pixels[frameIndex * frameWidth * frameHeight * 4 + i * 4 + 2] = copyPixels[i * 3 + 2];
			pixels[frameIndex * frameWidth * frameHeight * 4 + i * 4 + 3] = 255;
		}
	}
}

void testApp::writeDistorted() {
	drawImage.setFromPixels(outputPixels, frameWidth, frameHeight, OF_IMAGE_COLOR);
	drawImage.saveImage("distorted.tga", OF_IMAGE_QUALITY_BEST);
}

void testApp::calculateDrawSize(string path) {
	// Determine image size from the first frame.
	ofImage image;
	image.loadImage(path + "/frame0001.png");
	frameWidth = image.width;
	frameHeight = image.height;
	
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

/**
 * Thanks Mario!
 * @see http://incubator.quasimondo.com/processing/superfastblur.pde
 */
void testApp::fastBlur(unsigned char* pixels, int w, int h, int radius) {
	if (radius < 1) return;
	
	int wm=w-1;
	int hm=h-1;
	int wh=w*h;
	int div=radius+radius+1;
	int* r=new int[wh];
	int* g=new int[wh];
	int* b=new int[wh];
	int rsum,gsum,bsum,x,y,i,p,p1,p2,yp,yi,yw;
	int* vmin = new int[max(w,h)];
	int* vmax = new int[max(w,h)];
	int* dv=new int[256*div];
	for (i=0;i<256*div;i++){
		dv[i]=(i/div);
	}
	
	yw=yi=0;
	
	for (y=0;y<h;y++){
		rsum=gsum=bsum=0;
		for(i=-radius;i<=radius;i++){
			p=pixels[yi+min(wm,max(i,0))];
			rsum+=(p & 0xff0000)>>16;
			gsum+=(p & 0x00ff00)>>8;
			bsum+= p & 0x0000ff;
		}
		for (x=0;x<w;x++){
			
			r[yi]=dv[rsum];
			g[yi]=dv[gsum];
			b[yi]=dv[bsum];
			
			if(y==0){
				vmin[x]=min(x+radius+1,wm);
				vmax[x]=max(x-radius,0);
			}
			p1=pixels[yw+vmin[x]];
			p2=pixels[yw+vmax[x]];
			
			rsum+=((p1 & 0xff0000)-(p2 & 0xff0000))>>16;
			gsum+=((p1 & 0x00ff00)-(p2 & 0x00ff00))>>8;
			bsum+= (p1 & 0x0000ff)-(p2 & 0x0000ff);
			yi++;
		}
		yw+=w;
	}
	
	for (x=0;x<w;x++){
		rsum=gsum=bsum=0;
		yp=-radius*w;
		for(i=-radius;i<=radius;i++){
			yi=max(0,yp)+x;
			rsum+=r[yi];
			gsum+=g[yi];
			bsum+=b[yi];
			yp+=w;
		}
		yi=x;
		for (y=0;y<h;y++){
			pixels[yi]=0xff000000 | (dv[rsum]<<16) | (dv[gsum]<<8) | dv[bsum];
			if(x==0){
				vmin[y]=min(y+radius+1,hm)*w;
				vmax[y]=max(y-radius,0)*w;
			}
			p1=x+vmin[y];
			p2=x+vmax[y];
			
			rsum+=r[p1]-r[p2];
			gsum+=g[p1]-g[p2];
			bsum+=b[p1]-b[p2];
			
			yi+=w;
		}
	}
	
	delete[] r;
	delete[] g;
	delete[] b;
	delete[] vmin;
	delete[] vmax;
	delete[] dv;
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

