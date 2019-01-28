// client_cli_zscaler.cpp : This file contains the 'main' function. Program execution begins and ends there.
//



#include "pch.h"

#include <windows.h>
#include <WinUser.h>

#include <iostream>
#include <algorithm>

#include <openvr.h>

#include "vrinputemulator.h"

#include "vr_math.h"

#include <chrono>
#include <thread>

vr::IVRSystem* vr_pointer = NULL;

float calculate_offset(float base_height, float original);

int main()
{
	// init vr 

	vr::EVRInitError peError;
	vr::TrackedDevicePose_t pTrackedDevicePose;
	float base_height = 0.0;

	vr_pointer = vr::VR_Init(&peError, vr::EVRApplicationType::VRApplication_Background );

	if (peError != vr::VRInitError_None)
	{
		vr_pointer = NULL;
		printf("Unable to init VR runtime: %s \n",
			vr::VR_GetVRInitErrorAsEnglishDescription(peError));
		exit(EXIT_FAILURE);
	}

	// detect original player height, user should be wearing the headset at this time.

	vr_pointer->GetDeviceToAbsoluteTrackingPose(vr::ETrackingUniverseOrigin::TrackingUniverseStanding, (float) 0.0, &pTrackedDevicePose, 1);
	base_height = getHeight(&pTrackedDevicePose);

	// init emulator 
	vrinputemulator::DeviceOffsets device_offsets;

	vrinputemulator::VRInputEmulator inputEmulator;
	try
	{
		inputEmulator.connect(); // throws
	}
	catch (vrinputemulator::vrinputemulator_exception &e)
	{
		std::cout << "input emulator could not connect, maybe the driver is not yet registered";
		exit(EXIT_FAILURE);
	}
	inputEmulator.enableDeviceOffsets(0, true);

	// loop 

	auto indexL = vr_pointer->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_LeftHand);
	auto indexR = vr_pointer->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_RightHand);

	while (true) {

		bool is_exit = false;
		MSG uMsg;
		if (PeekMessage(&uMsg, NULL, 0, 0, PM_REMOVE) > 0) //Or use an if statement
		{
			is_exit = uMsg.message == WM_QUIT || uMsg.message == WM_CLOSE;
			std::cout << uMsg.message;
		}
		if (is_exit)
		{
			std::cout << "received exit signal";
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(5));

		vr_pointer->GetDeviceToAbsoluteTrackingPose(vr::ETrackingUniverseOrigin::TrackingUniverseStanding, (float) 0.0, &pTrackedDevicePose, 1);

		inputEmulator.getDeviceOffsets(0, device_offsets);
		float oldOffset = device_offsets.worldFromDriverTranslationOffset.v[1];

		float oldHeight = getHeight(&pTrackedDevicePose) - oldOffset;

		float newOffset = calculate_offset(base_height, oldHeight); /////////// calculate new offset here

		inputEmulator.setWorldFromDriverTranslationOffset
		(0 // HMD
			, vectorFromHeightd(newOffset)
		);

		// update controllers if available

		auto indexL = vr_pointer->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_LeftHand);
		auto indexR = vr_pointer->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_RightHand);

		if (vr_pointer->IsTrackedDeviceConnected(indexL))
		{
			inputEmulator.enableDeviceOffsets(indexL, true);
			inputEmulator.setWorldFromDriverTranslationOffset(indexL, vectorFromHeightd(newOffset));
		}

		if (vr_pointer->IsTrackedDeviceConnected(indexR))
		{
			inputEmulator.enableDeviceOffsets(indexR, true);
			inputEmulator.setWorldFromDriverTranslationOffset(indexR, vectorFromHeightd(newOffset));
		}

		std::cout << oldHeight << " + " << newOffset << "\r" ;
	}
	std::cout << '/n' << "left mainloop";


	// exit
	inputEmulator.enableDeviceOffsets(0, false);
	inputEmulator.enableDeviceOffsets(indexR, false);
	inputEmulator.enableDeviceOffsets(indexL, false);
	inputEmulator.disconnect();

	vr::VR_Shutdown();

	std::cout << '/n' << "done";

	return 0;
}

// H for Height in meter
#define H_STANDING (float) 1.75 // target height of player experience
// #define H_SEATED   (float) 1.35 // replaced by base_height param
//#define H_FLOOR    (float) 0.00
// #define H_DUCK_START (float) 1.30
// #define H_DUCK_END   (float) 0.95

// low value
//#define H_DUCKED   0.80

float calculate_offset(float base_height, float original)
{
	const float diff = H_STANDING - base_height /*H_SEATED*/;
	float duck_start = base_height - 0.05; 
	float duck_end = base_height - 0.40; 
	// bottom up
	if (original < duck_end)
	{
		return 0.0;
	}
	else if (original < duck_start)
	{
		return map_numeric(original, duck_end, duck_start, duck_end, duck_start + diff) - original;
	}
	else
	{
		return diff;
	}

};

