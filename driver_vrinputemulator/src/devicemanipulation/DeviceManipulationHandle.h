#pragma once

#include <openvr_driver.h>
#include <vrinputemulator_types.h>
#include <openvr_math.h>
//#include "utils/MovingAverageRingBuffer.h"
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

			MotionCompensationDeviceMode m_deviceMode = MotionCompensationDeviceMode::Default;

			vr::PropertyContainerHandle_t m_propertyContainerHandle = vr::k_ulInvalidPropertyContainer;

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

			MotionCompensationDeviceMode getDeviceMode() const
			{
				return m_deviceMode;
			}

			void setMotionCompensationDeviceMode(MotionCompensationDeviceMode DeviceMode);

			void ll_sendPoseUpdate(const vr::DriverPose_t& newPose);

			bool handlePoseUpdate(uint32_t& unWhichDevice, vr::DriverPose_t& newPose, uint32_t unPoseStructSize);

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
