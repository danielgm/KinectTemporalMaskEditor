#pragma once

#include "Layer.h"

/** A set of layers. */
struct Set {
	int index;
	string path;
	bool loaded;
	int layerCount;

	Layer* layers;

	Set() : layers(0) {
	}

	~Set() {
		if (layers) delete[] layers;
	}
};

