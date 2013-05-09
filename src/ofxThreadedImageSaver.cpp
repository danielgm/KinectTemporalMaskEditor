
#include "ofxThreadedImageSaver.h"

ofxThreadedImageSaver::ofxThreadedImageSaver() {
	this->frameIndex = 1;
}

void ofxThreadedImageSaver::threadedFunction() {
	string frameIndexStr = ofToString(frameIndex++);
	while (frameIndexStr.size() < 4) frameIndexStr = "0" + frameIndexStr;
	string filename = baseFilename + frameIndexStr + extension;
	
	if(lock()) {
		saveImage(filename);
		unlock();
	} else {
		printf("ofxThreadedImageSaver - cannot save %s cos I'm locked", filename.c_str());
	}
	stopThread();
}

void ofxThreadedImageSaver::saveThreaded(string filename) {
	int periodIndex = filename.find_last_of('.');
	this->baseFilename = filename.substr(0, periodIndex);
	this->extension = filename.substr(periodIndex, string::npos);
	
	startThread(false, false);   // blocking, verbose
}