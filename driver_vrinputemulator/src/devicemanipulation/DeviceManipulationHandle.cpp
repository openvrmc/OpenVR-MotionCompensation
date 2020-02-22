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
		#define VECTOR_ADD(lhs, rhs) \
		lhs[0] += rhs.v[0]; \
		lhs[1] += rhs.v[1]; \
		lhs[2] += rhs.v[2];


		DeviceManipulationHandle::DeviceManipulationHandle(const char* serial, vr::ETrackedDeviceClass eDeviceClass, void* driverPtr, void* driverHostPtr, int driverInterfaceVersion)
			: m_isValid(true), m_parent(ServerDriver::getInstance()), m_motionCompensationManager(m_parent->motionCompensation()), m_deviceDriverPtr(driverPtr), m_deviceDriverHostPtr(driverHostPtr),
			m_deviceDriverInterfaceVersion(driverInterfaceVersion), m_eDeviceClass(eDeviceClass), m_serialNumber(serial)
		{
		}

		bool DeviceManipulationHandle::handlePoseUpdate(uint32_t& unWhichDevice, vr::DriverPose_t& newPose, uint32_t unPoseStructSize)
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);

			if (m_deviceMode == 5)
			{ // motion compensation mode
				auto serverDriver = ServerDriver::getInstance();
				if (serverDriver)
				{
					if (newPose.poseIsValid && newPose.result == vr::TrackingResult_Running_OK)
					{
						m_motionCompensationManager._setMotionCompensationStatus(MotionCompensationStatus::Running);
						if (!m_motionCompensationManager._isMotionCompensationZeroPoseValid())
						{
							m_motionCompensationManager._setMotionCompensationZeroPose(newPose);
							serverDriver->sendReplySetMotionCompensationMode(true);
						}
						else
						{
							m_motionCompensationManager._updateMotionCompensationRefPose(newPose);
						}
					}
					else
					{
						if (!m_motionCompensationManager._isMotionCompensationZeroPoseValid())
						{
							setDefaultMode();
							serverDriver->sendReplySetMotionCompensationMode(false);
						}
						else
						{
							m_motionCompensationManager._setMotionCompensationStatus(MotionCompensationStatus::MotionRefNotTracking);
						}
					}
				}
				return true;

			}
			else
			{

				m_motionCompensationManager._applyMotionCompensation(newPose, this);

				return true;
			}
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

		bool _matchInputComponentName(const char* name, std::string& segment0, std::string& segment1, std::string& segment2, std::string& segment3)
		{
			boost::regex rgx("^/([^/]*)(/([^/]*))?(/([^/]*))?(/([^/]*))?");
			boost::smatch match;
			std::string text(name);
			if (boost::regex_search(text, match, rgx))
			{
				segment0 = match[1];
				segment1 = match[3];
				segment2 = match[5];
				segment3 = match[7];
				return true;
			}
			else
			{
				return false;
			}
		}

		int DeviceManipulationHandle::setDefaultMode()
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			auto res = _disableOldMode(0);
			if (res == 0)
			{
				m_deviceMode = 0;
			}
			return 0;
		}

		int DeviceManipulationHandle::setMotionCompensationMode()
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			auto res = _disableOldMode(5);
			auto serverDriver = ServerDriver::getInstance();
			if (res == 0 && serverDriver)
			{
				m_motionCompensationManager.enableMotionCompensation(true);
				m_motionCompensationManager.setMotionCompensationRefDevice(this);
				m_motionCompensationManager._setMotionCompensationStatus(MotionCompensationStatus::WaitingForZeroRef);
				m_deviceMode = 5;
			}
			return 0;
		}

		int DeviceManipulationHandle::_disableOldMode(int newMode)
		{
			if (m_deviceMode != newMode)
			{
				if (m_deviceMode == 5)
				{
					auto serverDriver = ServerDriver::getInstance();
					if (serverDriver)
					{
						m_motionCompensationManager.enableMotionCompensation(false);
						m_motionCompensationManager.setMotionCompensationRefDevice(nullptr);
					}
				}
				if (newMode == 5)
				{
					auto serverDriver = ServerDriver::getInstance();
					if (serverDriver)
					{
						m_motionCompensationManager._disableMotionCompensationOnAllDevices();
						m_motionCompensationManager.enableMotionCompensation(false);
						m_motionCompensationManager.setMotionCompensationRefDevice(nullptr);
					}
				}
			}
			return 0;
		}

	} // end namespace driver
} // end namespace vrinputemulator