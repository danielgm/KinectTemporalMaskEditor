#include "testApp.h"
#include "ofAppGlutWindow.h"

int main(){
	ofAppGlutWindow window;
	//ofSetupOpenGL(&window, 1280, 720, OF_WINDOW);
	ofSetupOpenGL(&window, 0, 0, OF_FULLSCREEN);
	ofRunApp(new testApp());
}
