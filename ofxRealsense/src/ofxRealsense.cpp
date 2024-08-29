#include "ofxRealsense.h"

ofxRealsense::ofxRealsense() {
	sensorParameterGroup.setName("Realsense not available");
}

ofxRealsense::~ofxRealsense() {


}

void ofxRealsense::setup(std::string camName, bool forceWait) {
	rs2::device_list devList = ctx.query_devices();
	rs2::device selected_device;

	if (devList.size() == 0) {
		std::cout << "Realsense_WARNING: No devices found!" << std::endl;

		if (forceWait) {
		
		//To help with the boilerplate code of waiting for a device to connect
		//The SDK provides the rs2::device_hub class
		rs2::device_hub device_hub(ctx);

		//Using the device_hub we can block the program until a device connects
		selected_device = device_hub.wait_for_device();
		
		}

	}
	else {
		std::cout << "Realsense_NOTICE: Found the following devices:" << std::endl;

		int index = 0;

		for (device dev : devList) {
			std::string devName = get_device_name(dev);
			std::cout << "\t" << index << " : " << devName << std::endl;
			
			//Found shard of the name; this must be it then
			if (devName.find(camName) != std::string::npos) {
				std::cout << "\tFound device, stop searching!" << std::endl;
				selected_device = dev;
				break;
			}
			
			index++;
		}

		if (selected_device) {

			std::cout << "Realsense_NOTICE: Found device continue" << std::endl;
			cdev = selected_device; //make the current device global (within class).
			
			//Start capture from this device
			pipeconfig.enable_device(cdev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
			pipeconfig.enable_stream(RS2_STREAM_COLOR, 1280, 720, RS2_FORMAT_RGB8);
			pipeconfig.enable_stream(RS2_STREAM_DEPTH, 1280, 720, RS2_FORMAT_Z16);

			pipeprofile = pipe.start(pipeconfig);
			
			//Iterate through its sensors:
			std::vector<sensor> cSens = get_sensors_from_a_device(cdev);
			
			for (auto i : cSens) {
				get_sensor_options(i);
			}

		}
		else {

			std::cout << "Realsense_WARNING: Didnt find device, continue" << std::endl;

		}

		
		
		
	}

	if (!sensorParameters.empty()) {

		sensorParameterGroup.setName(cdev.get_info(RS2_CAMERA_INFO_NAME));
		for (auto i : sensorParameters) {
			sensorParameterGroup.add(i);
		}
		
	}

}

void ofxRealsense::changeCameraSettings(std::vector<std::tuple<std::string, int>> sPack) {
	
	for (auto i : sPack) {
		std::cout << std::get<0>(i) << "," << std::get<1>(i) << std::endl;
	}

	std::vector<sensor> cSens = get_sensors_from_a_device(cdev);

	for (auto s : cSens) {
		for (int i = 0; i < static_cast<int>(RS2_OPTION_COUNT); i++)
		{
			rs2_option cOption = static_cast<rs2_option>(i);
			if (s.supports(cOption)) {

				for (auto gOpt : sPack) {
					
					std::string settingName = std::get<0>(gOpt);
					int settingVal = std::get<1>(gOpt);
					std::string optionName = rs2_option_to_string(cOption);

					if (optionName.find(settingName) != std::string::npos) {

						int cOptionVal = s.get_option(cOption);
						int cOptionMax = s.get_option_range(cOption).max;
						int cOptionMin = s.get_option_range(cOption).min;
						int cOptionStep = s.get_option_range(cOption).step;

						if (cOptionVal != settingVal && settingVal < cOptionMax && settingVal > cOptionMin) {

							try
							{
								s.set_option(cOption, (float)settingVal);
							}
							catch (const rs2::error& e)
							{
								// Some options can only be set while the camera is streaming,
								// and generally the hardware might fail so it is good practice to catch exceptions from set_option
								std::cerr << "Failed to set option " << cOption << ". (" << e.what() << ")" << std::endl;
							}

						}

					}

				}
				
			}
		}
	}

}

void ofxRealsense::update() {

	frameset curFrameset;
	if (pipe.poll_for_frames(&curFrameset)) {
		frame depthFrame = curFrameset.get_depth_frame();
		frame colorFrame = curFrameset.get_color_frame();
		
		if (!colorData.isAllocated()) {
			int w, h, err;
			rs2_get_video_stream_resolution(colorFrame.get_profile(), &w, &h,NULL);
			colorData.allocate(w,h,ofPixelFormat::OF_PIXELS_RGB);
		}

		colorData.setFromExternalPixels((unsigned char*)colorFrame.get_data(),colorData.getWidth(),colorData.getHeight(),ofPixelFormat::OF_PIXELS_RGB);

	}

	

}

ofImage ofxRealsense::getColorImg() {
	ofImage nImg;
	nImg.setFromPixels(colorData);
	nImg.rotate90(1);
	return nImg;

}

std::string ofxRealsense::get_device_name(const rs2::device& dev)
{
	// Each device provides some information on itself, such as name:
	std::string name = "Realsense_WARNING: Unknown Device";
	if (dev.supports(RS2_CAMERA_INFO_NAME))
		name = dev.get_info(RS2_CAMERA_INFO_NAME);

	// and the serial number of the device:
	std::string sn = "########";
	if (dev.supports(RS2_CAMERA_INFO_SERIAL_NUMBER))
		sn = std::string("#") + dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);

	return name + " " + sn;
}

std::string ofxRealsense::get_sensor_name(const rs2::sensor& sensor)
{
	// Sensors support additional information, such as a human readable name
	if (sensor.supports(RS2_CAMERA_INFO_NAME))
		return sensor.get_info(RS2_CAMERA_INFO_NAME);
	else
		return "Realsense_WARNING: Unknown Sensor";
}

std::vector<sensor> ofxRealsense::get_sensors_from_a_device(const rs2::device& dev)
{
	
	// Given a device, we can query its sensors using:
	std::vector<rs2::sensor> sensors = dev.query_sensors();

	std::cout << "Realsense_NOTICE: Device consists of " << sensors.size() << " sensors:\n" << std::endl;
	int index = 0;

	// We can now iterate the sensors and print their names
	for (rs2::sensor sensor : sensors)
	{
		std::cout << " \t" << index++ << " : " << get_sensor_name(sensor) << std::endl;
	}
		

	return  sensors;
}

std::vector<rs2_option> ofxRealsense::get_sensor_options(const rs2::sensor& sensor)
{
	std::vector<rs2_option> options;
	std::string sensorName = get_sensor_name(sensor);
	std::cout << sensorName <<" supports the following options:\n" << std::endl;
	std::cout << "________________________________________" << std::endl;

	// The following loop shows how to iterate over all available options
	// Starting from 0 until RS2_OPTION_COUNT (exclusive)
	for (int i = 0; i < static_cast<int>(RS2_OPTION_COUNT); i++)
	{
		rs2_option option_type = static_cast<rs2_option>(i);
		
		// First, verify that the sensor actually supports this option
		if (sensor.supports(option_type))
		{
			std::cout << "-------------------------------------" << std::endl;
			std::cout << i << ": " << option_type << std::endl;
			

			// Get a human readable description of the option
			const char* description = sensor.get_option_description(option_type);
			std::cout << "Desc: " << description << std::endl;

			// Get the current value of the option
			float current_value = sensor.get_option(option_type);
			option_range range = sensor.get_option_range(option_type);
			
			std::cout << "Cur: " << current_value << "\tMin: " << range.min << " \tMax: " <<range.max << "\tStep: "<< range.step << std::endl;

			//To change the value of an option, please follow the change_sensor_option() function
			ofParameter<int> nParameter;
			nParameter.set(rs2_option_to_string(option_type), (int)current_value, (int)range.min, (int)range.max);
			sensorParameters.push_back(nParameter);

			options.push_back(option_type);

		}
		else
		{
			//std::cout << " is not supported" << std::endl;
		}
	}

	std::cout << std::endl << std::endl;

	
	return options;
}
