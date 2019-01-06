// client_cli_zscaler.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <algorithm>

#include <openvr.h>

#include "vrinputemulator.h"

#include "vr_math.h"

#include <chrono>
#include <thread>

vr::IVRSystem* vr_pointer = NULL;

float calculate_offset(float original);

int main()
{
	// init vr 

	vr::EVRInitError peError;
	vr::TrackedDevicePose_t pTrackedDevicePose;

	vr_pointer = vr::VR_Init(&peError, vr::EVRApplicationType::VRApplication_Background );

	if (peError != vr::VRInitError_None)
	{
		vr_pointer = NULL;
		printf("Unable to init VR runtime: %s \n",
			vr::VR_GetVRInitErrorAsEnglishDescription(peError));
		exit(EXIT_FAILURE);
	}

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

	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(5));

		vr_pointer->GetDeviceToAbsoluteTrackingPose(vr::ETrackingUniverseOrigin::TrackingUniverseStanding, (float) 0.0, &pTrackedDevicePose, 1);

		inputEmulator.getDeviceOffsets(0, device_offsets);
		float oldOffset = device_offsets.worldFromDriverTranslationOffset.v[1];

		float oldHeight = getHeight(&pTrackedDevicePose) - oldOffset;
		float newOffset = calculate_offset(oldHeight);

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

	// exit

	vr::VR_Shutdown();
}

// H for Height in meter
#define H_STANDING (float) 1.75
#define H_SEATED   (float) 1.35
#define H_FLOOR    (float) 0.00
#define H_DUCK_START (float) 1.30
#define H_DUCK_END   (float) 0.95

// low value
//#define H_DUCKED   0.80

float calculate_offset(float original)
{
	const float diff = H_STANDING - H_SEATED;
	// bottom up
	if (original < H_DUCK_END)
	{
		return 0.0;
	}
	else if (original < H_DUCK_START)
	{
		return map_numeric(original, H_DUCK_END, H_DUCK_START, H_DUCK_END, H_DUCK_START + diff) - original;
	}
	else
	{
		return diff;
	}

};

