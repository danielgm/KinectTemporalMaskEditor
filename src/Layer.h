#pragma once

/** A layer of frames. */
struct Layer {
	int index;
	string path;
	int frameCount;

	/** For animation. */
	int frameOffset;

	// In milliseconds.
	long previousTime;

	unsigned char* pixels;

	Layer() : pixels(0) {
	}

	~Layer() {
		if (pixels) delete[] pixels;
	}
};

