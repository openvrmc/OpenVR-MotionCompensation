#include "IVRServerDriverHost006Hooks.h"

#include "../driver/ServerDriver.h"


namespace vrmotioncompensation
{
	namespace driver
	{
		HookData<IVRServerDriverHost006Hooks::trackedDeviceAdded_t> IVRServerDriverHost006Hooks::trackedDeviceAddedHook;
		HookData<IVRServerDriverHost006Hooks::trackedDevicePoseUpdated_t> IVRServerDriverHost006Hooks::trackedDevicePoseUpdatedHook;


		IVRServerDriverHost006Hooks::IVRServerDriverHost006Hooks(void* iptr)
		{
			if (!_isHooked)
			{
				CREATE_MH_HOOK(trackedDeviceAddedHook, _trackedDeviceAdded, "IVRServerDriverHost006::TrackedDeviceAdded", iptr, 0);
				CREATE_MH_HOOK(trackedDevicePoseUpdatedHook, _trackedDevicePoseUpdated, "IVRServerDriverHost006::TrackedDevicePoseUpdated", iptr, 1);
				_isHooked = true;
			}
		}

		IVRServerDriverHost006Hooks::~IVRServerDriverHost006Hooks()
		{
			if (_isHooked)
			{
				REMOVE_MH_HOOK(trackedDeviceAddedHook);
				REMOVE_MH_HOOK(trackedDevicePoseUpdatedHook);
				_isHooked = false;
			}
		}

		std::shared_ptr<InterfaceHooks> IVRServerDriverHost006Hooks::createHooks(void* iptr)
		{
			std::shared_ptr<InterfaceHooks> retval = std::shared_ptr<InterfaceHooks>(new IVRServerDriverHost006Hooks(iptr));
			return retval;
		}

		void IVRServerDriverHost006Hooks::trackedDevicePoseUpdatedOrig(void* _this, uint32_t unWhichDevice, const vr::DriverPose_t& newPose, uint32_t unPoseStructSize)
		{
			trackedDevicePoseUpdatedHook.origFunc(_this, unWhichDevice, newPose, unPoseStructSize);
		}

		bool IVRServerDriverHost006Hooks::_trackedDeviceAdded(void* _this, const char* pchDeviceSerialNumber, vr::ETrackedDeviceClass eDeviceClass, void* pDriver)
		{
			LOG(TRACE) << "IVRServerDriverHost006Hooks::_trackedDeviceAdded(" << _this << ", " << pchDeviceSerialNumber << ", " << eDeviceClass << ", " << pDriver << ")";
			
			serverDriver->hooksTrackedDeviceAdded(_this, 6, pchDeviceSerialNumber, eDeviceClass, pDriver);
			
			auto retval = trackedDeviceAddedHook.origFunc(_this, pchDeviceSerialNumber, eDeviceClass, pDriver);
			return retval;
		}

		// THOMAS: Here we create a copy of the original driver's pose; leaving that untouched. The copy is then forwarded to our manipulation and modified directly.
		// The modified pose then get returned to the original caller.
		void IVRServerDriverHost006Hooks::_trackedDevicePoseUpdated(void* _this, uint32_t unWhichDevice, const vr::DriverPose_t& newPose, uint32_t unPoseStructSize)
		{
			// Call rates:
			//
			// Vive HMD: 1120 calls/s
			// Vive Controller: 369 calls/s each
			//
			// Time is key. If we assume 1 HMD and 13 controllers, we have a total of  ~6000 calls/s. That's about 166 microseconds per call at 100% load.
			auto poseCopy = newPose;
			if (serverDriver->hooksTrackedDevicePoseUpdated(_this, 6, unWhichDevice, poseCopy, unPoseStructSize))
			{
				trackedDevicePoseUpdatedHook.origFunc(_this, unWhichDevice, poseCopy, unPoseStructSize);
			}
		}

	}
}