#include "IVRServerDriverHost005Hooks.h"

#include "../driver/ServerDriver.h"


namespace vrmotioncompensation
{
	namespace driver
	{
		HookData<IVRServerDriverHost005Hooks::trackedDeviceAdded_t> IVRServerDriverHost005Hooks::trackedDeviceAddedHook;
		HookData<IVRServerDriverHost005Hooks::trackedDevicePoseUpdated_t> IVRServerDriverHost005Hooks::trackedDevicePoseUpdatedHook;


		IVRServerDriverHost005Hooks::IVRServerDriverHost005Hooks(void* iptr)
		{
			if (!_isHooked)
			{
				CREATE_MH_HOOK(trackedDeviceAddedHook, _trackedDeviceAdded, "IVRServerDriverHost005::TrackedDeviceAdded", iptr, 0);
				CREATE_MH_HOOK(trackedDevicePoseUpdatedHook, _trackedDevicePoseUpdated, "IVRServerDriverHost005::TrackedDevicePoseUpdated", iptr, 1);
				_isHooked = true;
			}
		}

		IVRServerDriverHost005Hooks::~IVRServerDriverHost005Hooks()
		{
			if (_isHooked)
			{
				REMOVE_MH_HOOK(trackedDeviceAddedHook);
				REMOVE_MH_HOOK(trackedDevicePoseUpdatedHook);
				_isHooked = false;
			}
		}

		// Create hooks for the given pDriver pointer
		std::shared_ptr<InterfaceHooks> IVRServerDriverHost005Hooks::createHooks(void* iptr)
		{
			std::shared_ptr<InterfaceHooks> retval = std::shared_ptr<InterfaceHooks>(new IVRServerDriverHost005Hooks(iptr));
			return retval;
		}

		void IVRServerDriverHost005Hooks::trackedDevicePoseUpdatedOrig(void* _this, uint32_t unWhichDevice, const vr::DriverPose_t& newPose, uint32_t unPoseStructSize)
		{
			trackedDevicePoseUpdatedHook.origFunc(_this, unWhichDevice, newPose, unPoseStructSize);
		}

		bool IVRServerDriverHost005Hooks::_trackedDeviceAdded(void* _this, const char* pchDeviceSerialNumber, vr::ETrackedDeviceClass eDeviceClass, void* pDriver)
		{
			LOG(TRACE) << "IVRServerDriverHost005Hooks::_trackedDeviceAdded(" << _this << ", " << pchDeviceSerialNumber << ", " << eDeviceClass << ", " << pDriver << ")";
			
			serverDriver->hooksTrackedDeviceAdded(_this, 5, pchDeviceSerialNumber, eDeviceClass, pDriver);
			
			auto retval = trackedDeviceAddedHook.origFunc(_this, pchDeviceSerialNumber, eDeviceClass, pDriver);
			return retval;
		}

		void IVRServerDriverHost005Hooks::_trackedDevicePoseUpdated(void* _this, uint32_t unWhichDevice, const vr::DriverPose_t& newPose, uint32_t unPoseStructSize)
		{
			// Call rates:
			//
			// Vive HMD: 1120 calls/s
			// Vive Controller: 369 calls/s each
			//
			// Time is key. If we assume 1 HMD and 13 controllers, we have a total of  ~6000 calls/s. That's about 166 microseconds per call at 100% load.
			auto poseCopy = newPose;
			if (serverDriver->hooksTrackedDevicePoseUpdated(_this, 5, unWhichDevice, poseCopy, unPoseStructSize))
			{
				trackedDevicePoseUpdatedHook.origFunc(_this, unWhichDevice, poseCopy, unPoseStructSize);
			}
		}

	}
}