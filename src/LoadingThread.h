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

#include "Layer.h"
#include "Set.h"

class LoadingThread : public ofThread {
public:
	void load(Set* setArg, int frameWidthArg, int frameHeightArg) {
		if (loading) {
			cout << "Tried to load while already loading!" << endl;
			return;
		}

		set = setArg;
		frameWidth = frameWidthArg;
		frameHeight = frameHeightArg;

		// Thread safe.
		image.setUseTexture(false);
		
		loading = false;
		layerIndex = 0;
		frameIndex = 0;
		
		startThread();
	}

	bool isLoading() {
		return loading;
	}

	Set* getSet() {
		return set;
	}
	
private:
	Set* set;
	int frameWidth;
	int frameHeight;

	ofImage image;
	
	bool loading;

	int layerIndex;
	int frameIndex;
	
	void threadedFunction() {
		loading = true;
		while (isThreadRunning() && layerIndex < set->layerCount) {
			Layer* layer = &set->layers[layerIndex];
			layer->pixels = new unsigned char[layer->frameCount * frameWidth * frameHeight * 3];

			frameIndex = 0;
			while (frameIndex < layer->frameCount) {
				image.loadImage(layer->path + "/frame" + ofToString(frameIndex + 1, 0, 4, '0') + ".png");
			
				for (int i = 0; i < frameWidth * frameHeight * 3; i++) {
					layer->pixels[frameIndex * frameWidth * frameHeight * 3 + i] = image.getPixels()[i];
				}
				frameIndex++;
			}
			layerIndex++;
		}
		
		set->loaded = true;
		loading = false;
		stopThread();
		
		ofSendMessage("loaded");
	}
};

#endif /* defined(__KinectTemporalMaskEditor__LoadingThread__) */
