#include "IVRServerDriverHost004Hooks.h"

#include "../driver/ServerDriver.h"

namespace vrinputemulator
{
	namespace driver
	{
		HookData<IVRServerDriverHost004Hooks::trackedDeviceAdded_t> IVRServerDriverHost004Hooks::trackedDeviceAddedHook;
		HookData<IVRServerDriverHost004Hooks::trackedDevicePoseUpdated_t> IVRServerDriverHost004Hooks::trackedDevicePoseUpdatedHook;

		IVRServerDriverHost004Hooks::IVRServerDriverHost004Hooks(void* iptr)
		{
			if (!_isHooked)
			{
				CREATE_MH_HOOK(trackedDeviceAddedHook, _trackedDeviceAdded, "IVRServerDriverHost004::TrackedDeviceAdded", iptr, 0);
				CREATE_MH_HOOK(trackedDevicePoseUpdatedHook, _trackedDevicePoseUpdated, "IVRServerDriverHost004::TrackedDevicePoseUpdated", iptr, 1);
				_isHooked = true;
			}
		}

		IVRServerDriverHost004Hooks::~IVRServerDriverHost004Hooks()
		{
			if (_isHooked)
			{
				REMOVE_MH_HOOK(trackedDeviceAddedHook);
				REMOVE_MH_HOOK(trackedDevicePoseUpdatedHook);
				_isHooked = false;
			}
		}
		
		std::shared_ptr<InterfaceHooks> IVRServerDriverHost004Hooks::createHooks(void* iptr)
		{
			std::shared_ptr<InterfaceHooks> retval = std::shared_ptr<InterfaceHooks>(new IVRServerDriverHost004Hooks(iptr));
			return retval;
		}

		void IVRServerDriverHost004Hooks::trackedDevicePoseUpdatedOrig(void* _this, uint32_t unWhichDevice, const vr::DriverPose_t& newPose, uint32_t unPoseStructSize)
		{
			trackedDevicePoseUpdatedHook.origFunc(_this, unWhichDevice, newPose, unPoseStructSize);
		}

		bool IVRServerDriverHost004Hooks::_trackedDeviceAdded(void* _this, const char* pchDeviceSerialNumber, vr::ETrackedDeviceClass eDeviceClass, void* pDriver)
		{
			char* sn = (char*)pchDeviceSerialNumber;
			if ((sn >= (char*)0 && sn < (char*)0xff) || eDeviceClass < 0 || eDeviceClass > vr::ETrackedDeviceClass::TrackedDeviceClass_DisplayRedirect)
			{
				// SteamVR Vive driver bug, it's calling this function with random garbage
				LOG(ERROR) << "Not running _trackedDeviceAdded because of SteamVR driver bug.";
				return false;
			}
			LOG(TRACE) << "IVRServerDriverHost004Hooks::_trackedDeviceAdded(" << _this << ", " << pchDeviceSerialNumber << ", " << eDeviceClass << ", " << pDriver << ")";
			serverDriver->hooksTrackedDeviceAdded(_this, 4, pchDeviceSerialNumber, eDeviceClass, pDriver);
			auto retval = trackedDeviceAddedHook.origFunc(_this, pchDeviceSerialNumber, eDeviceClass, pDriver);
			return retval;
		}

		void IVRServerDriverHost004Hooks::_trackedDevicePoseUpdated(void* _this, uint32_t unWhichDevice, const vr::DriverPose_t& newPose, uint32_t unPoseStructSize)
		{
			// Call rates:
			//
			// Vive HMD: 1120 calls/s
			// Vive Controller: 369 calls/s each
			//
			// Time is key. If we assume 1 HMD and 13 controllers, we have a total of  ~6000 calls/s. That's about 166 microseconds per call at 100% load.
			auto poseCopy = newPose;
			if (serverDriver->hooksTrackedDevicePoseUpdated(_this, 4, unWhichDevice, poseCopy, unPoseStructSize))
			{
				trackedDevicePoseUpdatedHook.origFunc(_this, unWhichDevice, poseCopy, unPoseStructSize);
			}
		}

	}
}
