#pragma once
#pragma once

#include <openvr_driver.h>
#include <vrinputemulator_types.h>
#include <openvr_math.h>
#include "utils/KalmanFilter.h"
#include "utils/MovingAverageRingBuffer.h"
#include "../logging.h"
#include "../hooks/common.h"

// driver namespace
namespace vrinputemulator
{
	namespace driver
	{

		// forward declarations
		class ServerDriver;
		class InterfaceHooks;
		class MotionCompensationManager;


		// Stores manipulation information about an openvr device
		class DeviceManipulationHandle
		{
		private:
			bool m_isValid = false;
			ServerDriver* m_parent;
			MotionCompensationManager& m_motionCompensationManager;
			std::recursive_mutex _mutex;
			vr::ETrackedDeviceClass m_eDeviceClass = vr::TrackedDeviceClass_Invalid;
			uint32_t m_openvrId = vr::k_unTrackedDeviceIndexInvalid;
			std::string m_serialNumber;

			int m_deviceDriverInterfaceVersion = 0;
			void* m_deviceDriverPtr;
			void* m_deviceDriverHostPtr;
			void* m_driverInputPtr = nullptr;
			std::shared_ptr<InterfaceHooks> m_serverDriverHooks;
			std::shared_ptr<InterfaceHooks> m_controllerComponentHooks;

			int m_deviceMode = 0; // 0 .. default, 1 .. disabled, 2 .. redirect source, 3 .. redirect target, 4 .. swap mode, 5 .. motion compensation
			bool _disconnectedMsgSend = false;

			bool m_offsetsEnabled = false;
			vr::HmdQuaternion_t m_worldFromDriverRotationOffset = { 1.0, 0.0, 0.0, 0.0 };
			vr::HmdVector3d_t m_worldFromDriverTranslationOffset = { 0.0, 0.0, 0.0 };
			vr::HmdQuaternion_t m_driverFromHeadRotationOffset = { 1.0, 0.0, 0.0, 0.0 };
			vr::HmdVector3d_t m_driverFromHeadTranslationOffset = { 0.0, 0.0, 0.0 };
			vr::HmdQuaternion_t m_deviceRotationOffset = { 1.0, 0.0, 0.0, 0.0 };
			vr::HmdVector3d_t m_deviceTranslationOffset = { 0.0, 0.0, 0.0 };

			long long m_lastPoseTime = -1;
			bool m_lastPoseValid = false;
			vr::DriverPose_t m_lastPose;
			MovingAverageRingBuffer m_velMovingAverageBuffer;
			double m_lastPoseTimeOffset = 0.0;
			PosKalmanFilter m_kalmanFilter;

			vr::PropertyContainerHandle_t m_propertyContainerHandle = vr::k_ulInvalidPropertyContainer;

			int _disableOldMode(int newMode);

		public:
			DeviceManipulationHandle(const char* serial, vr::ETrackedDeviceClass eDeviceClass, void* driverPtr, void* driverHostPtr, int driverInterfaceVersion);

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

			void* driverPtr() const
			{
				return m_deviceDriverPtr;
			}
			void* driverHostPtr() const
			{
				return m_deviceDriverHostPtr;
			}
			void* driverInputPtr() const
			{
				return m_driverInputPtr;
			}
			void setDriverInputPtr(void* ptr)
			{
				m_driverInputPtr = ptr;
			}
			void setServerDriverHooks(std::shared_ptr<InterfaceHooks> hooks)
			{
				m_serverDriverHooks = hooks;
			}
			void setControllerComponentHooks(std::shared_ptr<InterfaceHooks> hooks)
			{
				m_controllerComponentHooks = hooks;
			}

			int deviceMode() const
			{
				return m_deviceMode;
			}
			int setDefaultMode();
			int setMotionCompensationMode();

			const vr::HmdQuaternion_t& worldFromDriverRotationOffset() const
			{
				return m_worldFromDriverRotationOffset;
			}
			vr::HmdQuaternion_t& worldFromDriverRotationOffset()
			{
				return m_worldFromDriverRotationOffset;
			}
			const vr::HmdVector3d_t& worldFromDriverTranslationOffset() const
			{
				return m_worldFromDriverTranslationOffset;
			}
			vr::HmdVector3d_t& worldFromDriverTranslationOffset()
			{
				return m_worldFromDriverTranslationOffset;
			}
			const vr::HmdQuaternion_t& driverFromHeadRotationOffset() const
			{
				return m_driverFromHeadRotationOffset;
			}
			vr::HmdQuaternion_t& driverFromHeadRotationOffset()
			{
				return m_driverFromHeadRotationOffset;
			}
			const vr::HmdVector3d_t& driverFromHeadTranslationOffset() const
			{
				return m_driverFromHeadTranslationOffset;
			}
			vr::HmdVector3d_t& driverFromHeadTranslationOffset()
			{
				return m_driverFromHeadTranslationOffset;
			}
			const vr::HmdQuaternion_t& deviceRotationOffset() const
			{
				return m_deviceRotationOffset;
			}
			vr::HmdQuaternion_t& deviceRotationOffset()
			{
				return m_deviceRotationOffset;
			}
			const vr::HmdVector3d_t& deviceTranslationOffset() const
			{
				return m_deviceTranslationOffset;
			}
			vr::HmdVector3d_t& deviceTranslationOffset()
			{
				return m_deviceTranslationOffset;
			}


			void ll_sendPoseUpdate(const vr::DriverPose_t& newPose);

			bool handlePoseUpdate(uint32_t& unWhichDevice, vr::DriverPose_t& newPose, uint32_t unPoseStructSize);

			PosKalmanFilter& kalmanFilter()
			{
				return m_kalmanFilter;
			}
			MovingAverageRingBuffer& velMovingAverage()
			{
				return m_velMovingAverageBuffer;
			}
			long long getLastPoseTime()
			{
				return m_lastPoseTime;
			}
			void setLastPoseTime(long long time)
			{
				m_lastPoseTime = time;
			}
			double getLastPoseTimeOffset()
			{
				return m_lastPoseTimeOffset;
			}
			void setLastPoseTimeOffset(double offset)
			{
				m_lastPoseTimeOffset = offset;
			}
			bool lastDriverPoseValid()
			{
				return m_lastPoseValid;
			}
			vr::DriverPose_t& lastDriverPose()
			{
				return m_lastPose;
			}
			void setLastDriverPoseValid(bool valid)
			{
				m_lastPoseValid = valid;
			}
			void setLastDriverPose(const vr::DriverPose_t& pose, long long time)
			{
				m_lastPose = pose;
				m_lastPoseTime = time;
				m_lastPoseValid = true;
			}

			void setPropertyContainer(vr::PropertyContainerHandle_t container)
			{
				m_propertyContainerHandle = container;
			}
			vr::PropertyContainerHandle_t propertyContainer()
			{
				return m_propertyContainerHandle;
			}

			void RunFrame();

		};


	} // end namespace driver
} // end namespace vrinputemulator