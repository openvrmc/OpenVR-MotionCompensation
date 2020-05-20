#pragma once

#include <stdint.h>


namespace vrmotioncompensation
{
	enum class MotionCompensationMode : uint32_t
	{
		Disabled = 0,
		ReferenceTracker = 1,
		Mover = 2,
	};

	enum class MotionCompensationDeviceMode : uint32_t
	{
		Default = 0,
		ReferenceTracker = 1,
		MotionCompensated = 2,
	};

	struct DeviceInfo
	{
		uint32_t OpenVRId;
		vr::ETrackedDeviceClass deviceClass;
		MotionCompensationDeviceMode deviceMode;
	};

} // end namespace vrmotioncompensation