OpenCV

Windows: Download OpenCV from http://opencv.org/downloads.html (prefer ver 2.4.9)

Linux: Install instructions https://help.ubuntu.com/community/OpenCV

//////////////////////////////////////////////////////////////////////////////////
Libwebsocket

Download libwebsockets from https://libwebsockets.org/trac/libwebsockets

Compile libwebsockets on Windows 
	Download CMake GUI for Windows at http://www.cmake.org/download/
	Run cmake-gui.exe
	Input the libwebsockets source path; choose the build path for libwebsockets
	Tick the "Advance" checkbox, select Visual Studio 2012 (64bit if applicable)
	Click "Configure". If there are SSL errors, uncheck all SSL checkboxes then click "Configure" again.
	Click "Build" to generate Visual Studio 2012 Solution for libwebsockets.
	Go to build folder. Run and compile the Visual Studio Solution (change to 64bit if select 64bit option in CMake configure)
	The generated lib files located in build/lib/Release(or Debug)
	
Compile libwebsockets on Linux
	$ cd /path/to/libwebsockets
	$ mkdir build
	$ cd build
	$ cmake ..
	$ make
	
More info to compile libwebsockets in libwebsockets/README.build.md file 

////////////////////////////////////////////////////////////////////
RapidJSON

Download rapidjson from https://github.com/miloyip/rapidjson

Rapidjson is a header-only C++ library, so it's ready to use without compiling.

///////////////////////////////////////////////////////////////////
Compile the main.cpp

On Windows using Visual Studio 2012
	Right click on VS Project, select Properties.
	Go to C/C++ --> General --> Additional Include Directories -> Edit --> 
		Add a path to /libwebsockets/lib
		Add a path to /rapidjson/include
		Add a path to /opencv/build/include. Then click Ok.
	Go to Linker --> General --> Additional Library Directories --> Edit --> 
		Add a path to /libwebsockets/build/lib/Release(or Debug)
		Add a path to /opencv/x86(or x64)/vc11/lib
	Go to Linker --> Input --> Additional Dependencies --> Add these file names, one on each line: 	
		websockets.lib
		opencv_calib3d249d.lib
		opencv_contrib249d.lib
		opencv_core249d.lib
		opencv_features2d249d.lib
		opencv_flann249d.lib
		opencv_gpu249d.lib
		opencv_highgui249d.lib
		opencv_imgproc249d.lib
		opencv_legacy249d.lib
		opencv_ml249d.lib
		opencv_nonfree249d.lib
		opencv_objdetect249d.lib
		opencv_ocl249d.lib
		opencv_photo249d.lib
		opencv_stitching249d.lib
		opencv_superres249d.lib
		opencv_ts249d.lib
		opencv_video249d.lib
		opencv_videostab249d.lib
	
	
On Linux
	$ g++ -I/path/to/libwebsockets/lib -I/path/to/rapidjson/include -I/path/to/opencv/include  -L/path/to/opencv/build/lib -L/path/to/libwebsockets/build/lib -g -o appOpenCV  main1.cpp -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_stitching -lwebsockets 

/////////////////////////////////////////////////////////////////////////////////////
Documentation

Server (main.cpp)

	bool isServer: Run in local or server mode
	bool isTesting: Run a trackbar window to filer color ranges.
	
	int H_MIN1,H_MAX1,S_MIN1,S_MAX1,V_MIN1,V_MAX1 values determine green color range.
	int H_MIN2,H_MAX2,S_MIN2,S_MAX2,V_MIN2,V_MAX2 values determine yellow color range.
	int H_MIN3,H_MAX3,S_MIN3,S_MAX3,V_MIN1,V_MAX3 values determine blue color range.
	These HSV values could be different in various lightning environment, so it's better to check color ranges again using the trackbar window.
	If more colors are needed to add, insert a function call checkColor("newColor", HSV, cameraFeed, newH_MIN, newH_MAX, newS_MIN, newS_MAX, newV_MIN, newV_MAX) in processFrame() with appropriate HSV value.
	

Client (index.html)
	A test html page to check server connection, input texts and click button to receive JSON data from server.
	