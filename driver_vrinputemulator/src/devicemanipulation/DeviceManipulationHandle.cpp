#include "DeviceManipulationHandle.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include "../driver/ServerDriver.h"
#include "../hooks/IVRServerDriverHost004Hooks.h"
#include "../hooks/IVRServerDriverHost005Hooks.h"

#undef WIN32_LEAN_AND_MEAN
#undef NOSOUND
#include <Windows.h>
// According to windows documentation mmsystem.h should be automatically included with Windows.h when WIN32_LEAN_AND_MEAN and NOSOUND are not defined
// But it doesn't work so I have to include it manually
#include <mmsystem.h>


namespace vrinputemulator
{
	namespace driver
	{
		DeviceManipulationHandle::DeviceManipulationHandle(const char* serial, vr::ETrackedDeviceClass eDeviceClass, void* driverPtr, void* driverHostPtr, int driverInterfaceVersion)
			: m_isValid(true), m_parent(ServerDriver::getInstance()), m_motionCompensationManager(m_parent->motionCompensation()), m_deviceDriverPtr(driverPtr), m_deviceDriverHostPtr(driverHostPtr),
			m_deviceDriverInterfaceVersion(driverInterfaceVersion), m_eDeviceClass(eDeviceClass), m_serialNumber(serial)
		{
		}

		bool DeviceManipulationHandle::handlePoseUpdate(uint32_t& unWhichDevice, vr::DriverPose_t& newPose, uint32_t unPoseStructSize)
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);

			if (m_deviceMode == MotionCompensationDeviceMode::ReferenceTracker)
			{ 
				//Check if the pose is valid to prefent unwanted jitter and movement
				if (newPose.poseIsValid && newPose.result == vr::TrackingResult_Running_OK)
				{
					//Set the Zero-Point for the reference tracker if not done yet
					if (!m_motionCompensationManager._isMotionCompensationZeroPoseValid())
					{						
						m_motionCompensationManager._setMotionCompensationZeroPose(newPose);
					}

					//Update reference tracker position
					m_motionCompensationManager._updateMotionCompensationRefPose(newPose);
				}

				return true;
			}
			else if (m_deviceMode == MotionCompensationDeviceMode::MotionCompensated)
			{
				m_motionCompensationManager._applyMotionCompensation(newPose, this);

				return true;
			}

			return true;
		}

		void DeviceManipulationHandle::ll_sendPoseUpdate(const vr::DriverPose_t& newPose)
		{
			if (m_deviceDriverInterfaceVersion == 4)
			{
				IVRServerDriverHost004Hooks::trackedDevicePoseUpdatedOrig(m_deviceDriverHostPtr, m_openvrId, newPose, sizeof(vr::DriverPose_t));
			}
			else if (m_deviceDriverInterfaceVersion == 5)
			{
				IVRServerDriverHost005Hooks::trackedDevicePoseUpdatedOrig(m_deviceDriverHostPtr, m_openvrId, newPose, sizeof(vr::DriverPose_t));
			}
		}

		void DeviceManipulationHandle::RunFrame()
		{
		}

		void DeviceManipulationHandle::setMotionCompensationDeviceMode(MotionCompensationDeviceMode DeviceMode)
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);

			m_deviceMode = DeviceMode;
		}
	} // end namespace driver
} // end namespace vrinputemulator