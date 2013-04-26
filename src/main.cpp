#include "testApp.h"
#include "ofAppGlutWindow.h"

int main(){
	ofAppGlutWindow window;
	ofSetupOpenGL(&window, 960, 540, OF_WINDOW);
	ofRunApp(new testApp());
}
