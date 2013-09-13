#include "testApp.h"

using namespace ofxCv;
using namespace cv;

void testApp::setup() {
	ofSetLogLevel(OF_LOG_VERBOSE);
	
	showHud = true;
	showMask = false;
	showGhost = true;
	recording = false;
	
	hudFont.loadFont("verdana.ttf", 12);
	
	nearThreshold = 204;
	farThreshold = 153;
	ghostThreshold = 196;
	fadeRate = 3;
	
	screenWidth = ofGetWindowWidth();
	screenHeight = ofGetWindowHeight();
	
	blurAmount = 3;
	
	string path = "input";
	countLayers(path);
	calculateDrawSize(path);
	
	frameCount = new int[layerCount];
	
	frameOffset = new int[layerCount];
	for (int layer = 0; layer < layerCount; layer++) {
		frameOffset[layer] = 0;
	}
    
	kinect.init(false, false);
	kinect.open();
	
	kinectAngle = 0;
	ofSetFrameRate(60);
	kinect.setCameraTiltAngle(kinectAngle);
	
	maskPixels = new unsigned char[kinect.width * kinect.height * 1];
	blurredPixels = new unsigned char[frameWidth * frameHeight * 4];
	inputPixels = new unsigned char*[layerCount];
	outputPixels = new unsigned char[frameWidth * frameHeight * 4];
	
	// Start the mask off black.
	for (int i = 0; i < kinect.width * kinect.height; i++) {
		maskPixels[i] = 0;
	}
	
	loadFrames(path, inputPixels);
	
	long bytes = 0;
	for (int layer = 0; layer < layerCount; layer++) {
		bytes += frameCount[layer] * frameWidth * frameHeight * 4;
	}
	
	cout << "---" << endl
	<< "Size:\t" << frameWidth << 'x' << frameHeight << endl
	<< "Memory: " << floor(bytes / 1024 / 1024) << " MB" << endl
	<< "---" << endl;
	
	previousTime = new long[layerCount];
	for (int layer = 0; layer < layerCount; layer++) {
		previousTime[layer] = ofGetSystemTime();
	}
}

void testApp::update() {
	long now = ofGetSystemTime();
	for (int layer = 0; layer < layerCount; layer++) {
		if (frameCount[layer] > 0) {
			// Increment the frame offset at the given FPS.
			int t = now - previousTime[layer];
			int d = floor(t / 1000.0 * frameOffsetFps);
			frameOffset[layer] += d;
			while (frameOffset[layer] > frameCount[layer]) frameOffset[layer] -= frameCount[layer];
			previousTime[layer] = now + floor(d * 1000.0 / frameOffsetFps) - t;
		}
	}
	
	if (kinect.isConnected()) {
		kinect.update();
		if (kinect.isFrameNew()) {
			// Write Kinect depth map pixels to the mask and fade.
			for (int i = 0; i < kinect.width * kinect.height; i++) {
				unsigned char k = kinect.getDepthPixels()[i];
				unsigned char m = MAX(0, maskPixels[i] - fadeRate);
				k = ofMap(CLAMP(k, farThreshold, nearThreshold), farThreshold, nearThreshold, 0, 255);
				blurredPixels[i] = maskPixels[i] = k > m ? k : m;
			}
			
			// FIXME: Should resize at this point. Currently just using kinect dimensions for frame size.
			assert(kinect.width == frameWidth);
			assert(kinect.height == frameHeight);
			fastBlur(blurredPixels, frameWidth, frameHeight, 5);
		}
	}
	else {
		int radius = 60;
		for (int offsetX = -radius; offsetX < radius; offsetX++) {
			for (int offsetY = -radius; offsetY < radius; offsetY++) {
				if (sqrt(offsetX * offsetX + offsetY * offsetY) < radius) {
					int x = mouseX + offsetX;
					int y = mouseY + offsetY;
					if (x >= 0 && x < frameWidth && y >= 0 && y < frameHeight) {
						blurredPixels[y * frameWidth + x] = min(255, blurredPixels[y * frameWidth + x] + 16);
					}
				}
			}
		}
		
		// FIXME: Should resize at this point. Currently just using kinect dimensions for frame size.
		assert(kinect.width == frameWidth);
		assert(kinect.height == frameHeight);
		fastBlur(blurredPixels, frameWidth, frameHeight, 3);
	}
	
	for (int i = 0; i < frameWidth * frameHeight; i++) {
		int layer = blurredPixels[i] * layerCount / 256;
		int frameIndex = ofMap(blurredPixels[i] - 256 * layer / layerCount, 0, 255 / layerCount, 0, frameCount[layer] - 1);
		
		frameIndex += frameOffset[layer];
		while (frameIndex >= frameCount[layer]) frameIndex -= frameCount[layer];
		
		for (int c = 0; c < 4; c++) {
			outputPixels[i * 4 + c] = inputPixels[layer][frameIndex * frameWidth * frameHeight * 4 + i * 4 + c];
			
			// FIXME: When showing ghost, the kinect pixels get referenced at the same dimensions
			// as the frame. This will cause problems if their sizes are different. Asserts commented
			// out for performance.
			//assert(kinect.width == frameWidth);
			//assert(kinect.height == frameHeight);
			if (showGhost && kinect.isConnected() && kinect.getDepthPixels()[i] > ghostThreshold) {
				outputPixels[i * 4 + c] = MIN(255, outputPixels[i * 4 + c] + 32);
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
	else {
		drawImage.setFromPixels(outputPixels, frameWidth, frameHeight, OF_IMAGE_COLOR_ALPHA);
		drawImage.draw((screenWidth - drawWidth)/2, (screenHeight - drawHeight)/2, drawWidth, drawHeight);
	}
		
	if (recording) {
		drawImage.saveImage(recordingPath + "/frame" + ofToString(recordingImageIndex++, 0, 4, '0') + ".png");
	}

	if (showHud) {
		long bytes = 0;
		for (int layer = 0; layer < layerCount; layer++) {
			bytes += frameCount[layer] * frameWidth * frameHeight * 4;
		}
		
		stringstream str;
		str << "Frame rate: " << ofToString(ofGetFrameRate(), 2) << endl
		<< "Frame size: " << frameWidth << 'x' << frameHeight << endl
		<< "Memory: " << floor(bytes / 1024 / 1024) << " MB" << endl
		<< "(G) Ghost: " << (showGhost ? "on" : "off") << endl
		<< "(T) Display: " << (showMask ? "mask" : "output") << endl
		<< "(J/K) Fade rate: " << fadeRate << endl
		<< "([/]) Tilt angle: " << kinectAngle << endl
		<< "(M) Recording: " << (recording ? "yes" : "no") << endl
		<< "(ESC) Quit" << endl;
		hudFont.drawString(str.str(), 32, 32);
		str.str(std::string());
	}
}

void testApp::exit() {
	delete[] maskPixels;
	delete[] blurredPixels;
	delete[] outputPixels;
	
	for (int layer = 0; layer < layerCount; layer++) {
		delete[] inputPixels[layer];
	}
	delete[] inputPixels;
}

void testApp::countLayers(string path) {
	layerCount = 0;
	ofFile file;
	while (file.doesFileExist(path + "/layer" + ofToString(layerCount + 1, 0, 2, '0'))) {
		layerCount++;
	}
}

void testApp::calculateDrawSize(string path) {
	// Determine image size from the first frame.
	ofImage image;
	image.loadImage(path + "/layer01/frame0001.png");
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

void testApp::loadFrames(string path, unsigned char** pixels) {
	ofFile file;
	ofImage image;
	for (int layer = 0; layer < layerCount; layer++) {
		
		// Count the number of frames.
		frameCount[layer] = 0;
		while (file.doesFileExist(path + "/layer" + ofToString(layer + 1, 0, 2, '0') + "/frame" + ofToString(frameCount[layer] + 1, 0, 4, '0') + ".png")) {
			frameCount[layer]++;
		}
		
		inputPixels[layer] = new unsigned char[frameCount[layer] * frameWidth * frameHeight * 4];
		for (int frameIndex = 0; frameIndex < frameCount[layer]; frameIndex++) {
			image.loadImage(path + "/layer" + ofToString(layer + 1, 0, 2, '0') + "/frame" + ofToString(frameIndex + 1, 0, 4, '0') + ".png");
			
			unsigned char* copyPixels = image.getPixels();
			for (int i = 0; i < frameWidth * frameHeight; i++) {
				pixels[layer][frameIndex * frameWidth * frameHeight * 4 + i * 4 + 0] = copyPixels[i * 3 + 0];
				pixels[layer][frameIndex * frameWidth * frameHeight * 4 + i * 4 + 1] = copyPixels[i * 3 + 1];
				pixels[layer][frameIndex * frameWidth * frameHeight * 4 + i * 4 + 2] = copyPixels[i * 3 + 2];
				pixels[layer][frameIndex * frameWidth * frameHeight * 4 + i * 4 + 3] = 255;
			}
		}
	}
}

void testApp::writeDistorted() {
	drawImage.saveImage("distorted.png");
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

