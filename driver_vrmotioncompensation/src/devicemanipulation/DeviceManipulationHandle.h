#pragma once

#include <openvr_driver.h>
#include <vrmotioncompensation_types.h>
#include "../hooks/common.h"


// driver namespace
namespace vrmotioncompensation
{
	namespace driver
	{
		// forward declarations
		class ServerDriver;
		class InterfaceHooks;
		class MotionCompensationManager;


		// Stores manipulation information about an Open VR device
		class DeviceManipulationHandle
		{
		private:
			bool m_isValid = false;
			ServerDriver* m_parent;
			MotionCompensationManager& m_motionCompensationManager;
			vr::ETrackedDeviceClass m_eDeviceClass = vr::TrackedDeviceClass_Invalid;
			uint32_t m_openvrId = vr::k_unTrackedDeviceIndexInvalid;
			std::string m_serialNumber;

			std::shared_ptr<InterfaceHooks> m_serverDriverHooks;

			MotionCompensationDeviceMode m_deviceMode = MotionCompensationDeviceMode::Default;

		public:
			DeviceManipulationHandle(const char* serial, vr::ETrackedDeviceClass eDeviceClass);

			bool isValid() const
			{
				return m_isValid;
			}

			vr::ETrackedDeviceClass deviceClass() const
			{
				return m_eDeviceClass;
			}

			uint32_t openvrId() const
			{
				return m_openvrId;
			}

			void setOpenvrId(uint32_t id)
			{
				m_openvrId = id;
			}

			const std::string& serialNumber()
			{
				return m_serialNumber;
			}

			void setServerDriverHooks(std::shared_ptr<InterfaceHooks> hooks)
			{
				m_serverDriverHooks = hooks;
			}

			MotionCompensationDeviceMode getDeviceMode() const
			{
				return m_deviceMode;
			}

			void setMotionCompensationDeviceMode(MotionCompensationDeviceMode DeviceMode);

			bool handlePoseUpdate(uint32_t& unWhichDevice, vr::DriverPose_t& newPose, uint32_t unPoseStructSize);

			//vr::HmdVector3d_t ToEulerAngles(vr::HmdQuaternion_t q);
		};
	} // end namespace driver
} // end namespace vrmotioncompensation