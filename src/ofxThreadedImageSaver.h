
#include "ofMain.h"
#include "ofxThread.h"

/**
 * Based on code by OF user 'memo'.
 * @see http://forum.openframeworks.cc/index.php?topic=1687.0
 */
class ofxThreadedImageSaver : public ofxThread, public ofImage {
public:
	ofxThreadedImageSaver();
    void threadedFunction();
    void saveThreaded(string baseFilename);
	
	int frameIndex;
    string baseFilename;
	string extension;
};