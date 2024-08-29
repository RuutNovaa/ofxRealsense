#pragma once
#ifndef OFX_REALSENSE_ADDON
#define OFX_REALSENSE_ADDON



#include "ofMain.h"
#include <rs.hpp>

using namespace rs2;

class ofxRealsense {

public:
	ofxRealsense();
	~ofxRealsense();

	void setup(std::string camName, bool forceWait=false);
	void update();

	void changeCameraSettings(std::vector<std::tuple<std::string, int>> sPack);

	std::vector<ofParameterGroup> rsCam;

	ofImage getColorImg();
	ofParameterGroup sensorParameterGroup;

private:
	std::string get_device_name(const rs2::device& dev);
	std::vector<sensor> get_sensors_from_a_device(const rs2::device& dev);
	std::string get_sensor_name(const rs2::sensor& sensor);
	std::vector<rs2_option> get_sensor_options(const rs2::sensor& sensor);

	context ctx;
	device cdev;
	pipeline pipe;
	pipeline_profile pipeprofile;
	config pipeconfig;

	ofPixels colorData;
	std::vector<ofParameter<int>> sensorParameters;
	

	
};

#endif // !