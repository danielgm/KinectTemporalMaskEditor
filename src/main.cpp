#include "testApp.h"
#include "ofAppGlutWindow.h"

int main(){
	ofAppGlutWindow window;
	ofSetupOpenGL(&window, 640, 480, OF_WINDOW);
	//ofSetupOpenGL(&window, 0, 0, OF_FULLSCREEN);
	ofRunApp(new testApp());
}
