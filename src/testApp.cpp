#include "testApp.h"

using namespace ofxCv;
using namespace cv;

void testApp::setup() {
	ofSetLogLevel(OF_LOG_VERBOSE);
	
	// Defaults.
	nearThreshold = 255;
	farThreshold = 64;
	ghostThreshold = 196;
	fadeRate = 3;
	kinectAngle = 15;
	autoSetAdvance = true;
	fadeInDuration = 1000;
	setDuration = 5000;
	fadeOutDuration = 1000;
	showGhost = true;
	blurAmount = 3;
	frameOffsetFps = 30;

	readConfig();
	
	showHud = true;
	showMask = false;
	recording = false;
	loading = false;
	
	hudFont.loadFont("verdana.ttf", 12);
	
	screenWidth = ofGetScreenWidth();
	screenHeight = ofGetScreenHeight();
	
	kinect.init(true, false, false);
	kinect.open();
	
	ofSetFrameRate(60);
	kinect.setCameraTiltAngle(kinectAngle);
	
	calculateDrawSize();

	int bytes = 0;
	int totalLayers = 0;

	setCount = countSets();
	sets = new Set[setCount];
	for (int setIndex = 0; setIndex < setCount; setIndex++) {
		Set* set = &sets[setIndex];
		set->index = setIndex;
		set->path = "set" + ofToString(setIndex + 1, 0, 2, '0');
		set->loaded = false;
		set->layerCount = countLayers(set);

		cout << "Path: " << set->path << endl;
		cout << "Layer count: " << set->layerCount << endl;

		set->layers = new Layer[set->layerCount];
		for (int layerIndex = 0; layerIndex < set->layerCount; layerIndex++) {
			Layer* layer = &set->layers[layerIndex];
			layer->index = layerIndex;
			layer->path = set->path + "/layer" + ofToString(layerIndex + 1, 0, 2, '0');
			layer->frameCount = countFrames(layer);
			layer->frameOffset = 0;

			cout << "Path: " << layer->path << endl;
			cout << "Frame count: " << layer->frameCount << endl;

			bytes += layer->frameCount * frameWidth * frameHeight * 3;
			totalLayers++;
		}
	}
	currSetIndex = 0;

	maskPixels = new unsigned char[kinect.width * kinect.height * 1];
	blurredPixels = new unsigned char[frameWidth * frameHeight * 3];
	outputPixels = new unsigned char[frameWidth * frameHeight * 3];
	
	// Start the mask off black.
	for (int i = 0; i < kinect.width * kinect.height; i++) {
		maskPixels[i] = 0;
	}

	cout << "---" << endl
	<< "Frame size: " << frameWidth << 'x' << frameHeight << endl
	<< "Total sets: " << setCount << endl
	<< "Total layers: " << totalLayers << endl
	<< "Total memory: " << floor(bytes / 1024 / 1024) << " MB" << endl
	<< "---" << endl;

	// Set previous time for animation as last step.
	for (int setIndex = 0; setIndex < setCount; setIndex++) {
		Set* set = &sets[setIndex];
		for (int layerIndex = 0; layerIndex < set->layerCount; layerIndex++) {
			set->layers[layerIndex].previousTime = ofGetSystemTime();
		}
	}
	setStartTime = ofGetSystemTime();
}

void testApp::update() {
	long now = ofGetSystemTime();
	if (autoSetAdvance && now - setStartTime > fadeInDuration + setDuration + fadeOutDuration) {
		setStartTime = now;
		currSetIndex++;
		if (currSetIndex >= setCount) currSetIndex = 0;
	}

	Set* currSet = &sets[currSetIndex];
	if (currSet->loaded) {
		updateFrameOffsets(currSet, now);
		
		if (kinect.isConnected()) {
			kinect.update();
			if (kinect.isFrameNew()) {
				// Write Kinect depth map pixels to the mask and fade.
				for (int x = 0; x < kinect.width; x++) {
					for (int y = 0; y < kinect.height; y++) {
						int i = y * kinect.width + x;
						unsigned char k = kinect.getDepthPixels()[y * kinect.width + kinect.width - x - 1];
						unsigned char m = MAX(0, maskPixels[i] - fadeRate);
						k = ofMap(CLAMP(k, farThreshold, nearThreshold), farThreshold, nearThreshold, 0, 255);
						blurredPixels[i] = maskPixels[i] = k > m ? k : m;
					}
				}
				
				// FIXME: Should resize at this point. Currently just using kinect dimensions for frame size.
				assert(kinect.width == frameWidth);
				assert(kinect.height == frameHeight);
				fastBlur(blurredPixels, frameWidth, frameHeight, 5);
			}
		}
		else {
			// Mouse version if Kinect not present.
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
		
		for (int x = 0; x < kinect.width; x++) {
			for (int y = 0; y < kinect.height; y++) {
				int i = y * kinect.width + x;
				int layerIndex = blurredPixels[i] * currSet->layerCount / 256;
				Layer* layer = &currSet->layers[layerIndex];
				int frameIndex = ofMap(blurredPixels[i] - 256 * layerIndex / currSet->layerCount, 0, 255 / currSet->layerCount, 0, layer->frameCount - 1);
				
				frameIndex += layer->frameOffset;
				while (frameIndex >= layer->frameCount) frameIndex -= layer->frameCount;
				
				for (int c = 0; c < 3; c++) {
					outputPixels[i * 3 + c] = layer->pixels[frameIndex * frameWidth * frameHeight * 3 + i * 3 + c];
					
					float d = now - setStartTime;
					float maskMultiplier = 1;
					if (d < fadeInDuration) {
						maskMultiplier = d / fadeInDuration;
					}
					else if (autoSetAdvance && d > fadeInDuration + setDuration) {
						maskMultiplier = (fadeInDuration + setDuration + fadeOutDuration - d) / fadeOutDuration;
					}
					outputPixels[i * 3 + c] = outputPixels[i * 3 + c] * maskMultiplier
					+ maskPixels[i] * (1 - maskMultiplier) * 0.5;
					
					// FIXME: When showing ghost, the kinect pixels get referenced at the same dimensions
					// as the frame. This will cause problems if their sizes are different. Asserts commented
					// out for performance.
					//assert(kinect.width == frameWidth);
					//assert(kinect.height == frameHeight);
					if (showGhost && kinect.isConnected() && kinect.getDepthPixels()[y * kinect.width + kinect.width - x - 1] > ghostThreshold) {
						outputPixels[i * 3 + c] = MIN(255, outputPixels[i * 3 + c] + 32);
					}
				}
			}
		}
	}
	else if (!loading) {
		loader.load(currSet, frameWidth, frameHeight);
		loading = true;
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
		drawImage.setFromPixels(outputPixels, frameWidth, frameHeight, OF_IMAGE_COLOR);
		drawImage.draw((screenWidth - drawWidth)/2, (screenHeight - drawHeight)/2, drawWidth, drawHeight);
	}
		
	if (recording) {
		drawImage.saveImage(recordingPath + "/frame" + ofToString(recordingImageIndex++, 0, 4, '0') + ".png");
	}

	if (showHud) {
		int bytes = 0;
		int totalLayers = 0;

		for (int setIndex = 0; setIndex < setCount; setIndex++) {
			Set* set = &sets[setIndex];
			for (int layerIndex = 0; layerIndex < set->layerCount; layerIndex++) {
				Layer* layer = &set->layers[layerIndex];
				bytes += layer->frameCount * frameWidth * frameHeight * 3;
				totalLayers++;
			}
		}
		
		stringstream ss;
		ss << "Frame rate: " << ofToString(ofGetFrameRate(), 2) << endl
		<< "Frame size: " << frameWidth << 'x' << frameHeight << endl
		<< "Sets: " << setCount << endl
		<< "Layers: " << totalLayers << endl
		<< "Memory: " << floor(bytes / 1024 / 1024) << " MB" << endl
		<< "(A) Auto-advance: " << (autoSetAdvance ? "yes" : "no") << endl
		<< "(,/.) Duration: " << setDuration << " ms" << endl
		<< "(G) Ghost: " << (showGhost ? "on" : "off") << endl
		<< "(T) Display: " << (showMask ? "mask" : "output") << endl
		<< "(J/K) Mask fade rate: " << fadeRate << endl
		<< "(N/B) Near threshold: " << nearThreshold << endl
		<< "(F/D) Far threshold: " << farThreshold << endl
		<< "([/]) Tilt angle: " << kinectAngle << endl
		<< "(M) Recording: " << (recording ? "yes" : "no") << endl
		<< "(ESC) Quit" << endl;
		hudFont.drawString(ss.str(), 32, 32);
		ss.str(std::string());
	}
}

void testApp::exit() {
	delete[] maskPixels;
	delete[] blurredPixels;
	delete[] outputPixels;
	delete[] sets;
}

int testApp::countSets() {
	int n = 0;
	ofFile file;
	while (file.doesFileExist("set" + ofToString(n + 1, 0, 2, '0'))) n++;
	return n;
}

int testApp::countLayers(Set* set) {
	int n = 0;
	ofFile file;
	while (file.doesFileExist(set->path + "/layer" + ofToString(n + 1, 0, 2, '0'))) n++;
	return n;
}

int testApp::countFrames(Layer* layer) {
	int n = 0;
	ofFile file;
	while (file.doesFileExist(layer->path + "/frame" + ofToString(n + 1, 0, 4, '0') + ".png")) n++;
	return n;
}

void testApp::calculateDrawSize() {
	// Determine image size from the first frame.
	ofImage image;
	image.loadImage("set01/layer01/frame0001.png");
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

void testApp::updateFrameOffsets(Set* set, long now) {
	for (int layerIndex = 0; layerIndex < set->layerCount; layerIndex++) {
		Layer* layer = &set->layers[layerIndex];
		
		// Increment the frame offset at the given FPS.
		int t = now - layer->previousTime;
		int d = floor(t / 1000.0 * frameOffsetFps);
		layer->frameOffset += d;
		while (layer->frameOffset > layer->frameCount) layer->frameOffset -= layer->frameCount;
		layer->previousTime = now + floor(d * 1000.0 / frameOffsetFps) - t;
	}
}

void testApp::writeDistorted() {
	drawImage.saveImage("distorted.png");
}

void testApp::readConfig() {
	ofBuffer b = ofBufferFromFile("config.properties");
	while (!b.isLastLine()) {
		string line = b.getNextLine();
		vector<string> parts = ofSplitString(line, "=");
		
		if (parts[0] == "nearThreshold") nearThreshold = ofToInt(parts[1]);
		if (parts[0] == "farThreshold") farThreshold = ofToInt(parts[1]);
		if (parts[0] == "ghostThreshold") ghostThreshold = ofToInt(parts[1]);
		if (parts[0] == "fadeRate") fadeRate = ofToInt(parts[1]);
		if (parts[0] == "kinectAngle") kinectAngle = ofToInt(parts[1]);
		if (parts[0] == "autoSetAdvance") autoSetAdvance = ofToInt(parts[1]) > 0;
		if (parts[0] == "fadeInDuration") fadeInDuration = ofToInt(parts[1]);
		if (parts[0] == "setDuration") setDuration = ofToInt(parts[1]);
		if (parts[0] == "fadeOutDuration") fadeOutDuration = ofToInt(parts[1]);
		if (parts[0] == "showGhost") showGhost = ofToInt(parts[1]) > 0;
		if (parts[0] == "blurAmount") blurAmount = ofToInt(parts[1]);
		if (parts[0] == "frameOffsetFps") frameOffsetFps = ofToInt(parts[1]);
	}
}

void testApp::writeConfig() {
	ofFile f;
	f.open("config.properties", ofFile::WriteOnly, false);
	f << "nearThreshold=" << nearThreshold << endl;
	f << "farThreshold=" << farThreshold << endl;
	f << "ghostThreshold=" << ghostThreshold << endl;
	f << "fadeRate=" << fadeRate << endl;
	f << "kinectAngle=" << kinectAngle << endl;
	f << "autoSetAdvance=" << autoSetAdvance << endl;
	f << "fadeInDuration=" << fadeInDuration << endl;
	f << "setDuration=" << 	setDuration << endl;
	f << "fadeOutDuration=" << 	fadeOutDuration << endl;
	f << "showGhost=" << 	showGhost << endl;
	f << "blurAmount=" << 	blurAmount << endl;
	f << "frameOffsetFps=" << 	frameOffsetFps << endl;
	f.close();
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

		case OF_KEY_RETURN:
			currSetIndex++;
			if (currSetIndex >= setCount) currSetIndex = 0;
			setStartTime = ofGetSystemTime();
			break;
			
		case 'w':
			writeDistorted();
			break;
			
		case 'g':
			showGhost = !showGhost;
			writeConfig();
			break;
			
		case 't':
			showMask = !showMask;
			break;
			
		case 'j':
			fadeRate--;
			if (fadeRate < 0) fadeRate = 0;
			cout << "Fade rate: " << fadeRate << endl;
			writeConfig();
			break;
			
		case 'k':
			fadeRate++;
			if (fadeRate > 255) fadeRate = 255;
			cout << "Fade rate: " << fadeRate << endl;
			writeConfig();
			break;
			
		case 'n':
			nearThreshold++;
			if (nearThreshold > 255) nearThreshold = 255;
			cout << "Near threshold: " << nearThreshold << endl;
			writeConfig();
			break;
			
		case 'b':
			nearThreshold--;
			if (nearThreshold <= farThreshold) nearThreshold = farThreshold + 1;
			cout << "Near threshold: " << nearThreshold << endl;
			writeConfig();
			break;
			
		case 'f':
			farThreshold++;
			if (farThreshold >= nearThreshold) farThreshold = nearThreshold - 1;
			cout << "Far threshold: " << farThreshold << endl;
			writeConfig();
			break;
			
		case 'd':
			farThreshold--;
			if (farThreshold < 0) farThreshold = 0;
			cout << "Far threshold: " << farThreshold << endl;
			writeConfig();
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
			writeConfig();
			break;
			
		case ']':
			kinectAngle += 0.5;
			kinect.setCameraTiltAngle(kinectAngle);
			cout << "Tilt angle: " << kinectAngle << endl;
			writeConfig();
			break;
		
		case ',':
			setDuration -= 2500;
			if (setDuration < 2500) setDuration = 2500;
			cout << "Set duration: " << setDuration << " ms" << endl;
			writeConfig();
			break;
		
		case '.':
			setDuration += 2500;
			cout << "Set duration: " << setDuration << " ms" << endl;
			writeConfig();
			break;

		case 'a':
			autoSetAdvance = !autoSetAdvance;
			setStartTime = ofGetSystemTime() - fadeInDuration;
			cout << "Auto advance: " << (autoSetAdvance ? "yes" : "no") << endl;
			writeConfig();
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
		loading = false;

		if (setCount > 1) {
			// Unload the previous set.
			int prevSetIndex = currSetIndex - 1;
			if (prevSetIndex < 0) prevSetIndex = setCount - 1;
			cout << "Unloading set" << ofToString(prevSetIndex + 1, 0, 2, '0') << endl;

			Set* prevSet = &sets[prevSetIndex];
			for (int layerIndex = 0; layerIndex < prevSet->layerCount; layerIndex++) {
				delete[] prevSet->layers[layerIndex].pixels;
				prevSet->layers[layerIndex].pixels = 0;
			}
			prevSet->loaded = false;

			int nextSetIndex = currSetIndex + 1;
			if (nextSetIndex >= setCount) nextSetIndex = 0;
			cout << "Loading set" << ofToString(nextSetIndex + 1, 0, 2, '0') << endl;
			loader.load(&sets[nextSetIndex], frameWidth, frameHeight);
		}
	}
}

void testApp::dragEvent(ofDragInfo dragInfo) {
}

