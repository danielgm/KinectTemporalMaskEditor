//
//  LoadingThread.h
//  KinectTemporalMaskEditor
//
//  Created by Daniel McLaren on 13-05-31.
//
//

#ifndef __KinectTemporalMaskEditor__LoadingThread__
#define __KinectTemporalMaskEditor__LoadingThread__

#include "ofThread.h"
#include <iostream>


/**
 * IMPORTANT: This class allocates pixels to store the movie clip frames. It's up
 * to the client class to clean the variables.
 */
class LoadingThread : public ofThread {
public:
	void load(string folderArg) {
		// Thread safe.
		image.setUseTexture(false);
		
		folder = folderArg;
		
		frameIndex = 0;
		
		frameCount = 0;
		frameWidth = 0;
		frameHeight = 0;
		
		ofFile file;
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
		
		if (frameWidth <= 0 || frameHeight <= 0) return;
		
		cout << "Loading " << frameCount << " frames at " << frameWidth << "x" << frameHeight << "... ";
		
		if (pixels) delete[] pixels;
		pixels = new unsigned char[frameCount * frameWidth * frameHeight * 4];
		
		startThread();
	}
	
	int getFrameCount() {
		return frameCount;
	}
	
	int getFrameWidth() {
		return frameWidth;
	}
	
	int getFrameHeight() {
		return frameHeight;
	}
	
	int getFramesLoaded() {
		return frameIndex;
	}
	
	unsigned char* getPixels() {
		return pixels;
	}

private:
	string folder;
	
	ofImage image;
	unsigned char* copyPixels;
	unsigned char* pixels;
	
	int frameIndex;
	char indexString[5];
	
	int frameCount;
	int frameWidth;
	int frameHeight;
	
    void threadedFunction() {
		while (isThreadRunning() && frameIndex < frameCount) {
			sprintf(indexString, "%04d", frameIndex + 1);
			image.loadImage(folder + "/frame" + indexString + ".jpg");
			
			copyPixels = image.getPixels();
			for (int i = 0; i < frameWidth * frameHeight; i++) {
				pixels[frameIndex * frameWidth * frameHeight * 4 + i * 4 + 0] = copyPixels[i * 3 + 0];
				pixels[frameIndex * frameWidth * frameHeight * 4 + i * 4 + 1] = copyPixels[i * 3 + 1];
				pixels[frameIndex * frameWidth * frameHeight * 4 + i * 4 + 2] = copyPixels[i * 3 + 2];
				pixels[frameIndex * frameWidth * frameHeight * 4 + i * 4 + 3] = 255;
			}
			
			frameIndex++;
		}
		
		cout << "done." << endl;
		
		ofSendMessage("loaded");
	}
};

#endif /* defined(__KinectTemporalMaskEditor__LoadingThread__) */
