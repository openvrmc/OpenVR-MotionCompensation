#pragma once

#include <memory>
#include <mutex>
#include <queue>
#include <openvr_driver.h>
#include <vrmotioncompensation_types.h>
#include "../hooks/common.h"
#include "../logging.h"
#include "../com/shm/driver_ipc_shm.h"
#include "../devicemanipulation/MotionCompensationManager.h"

// driver namespace
namespace vrmotioncompensation
{
	namespace driver
	{		
		// forward declarations
		class ServerDriver;
		class InterfaceHooks;
		class VirtualDeviceDriver;
		class DeviceManipulationHandle;


		/**
		* Implements the IServerTrackedDeviceProvider interface.
		*
		* Its the main entry point of the driver. It's a singleton which manages all devices owned by this driver,
		* and also handles the whole "hacking into OpenVR" stuff.
		*/
		class ServerDriver : public vr::IServerTrackedDeviceProvider
		{
		public:
			ServerDriver();
			virtual ~ServerDriver();

			//// from IServerTrackedDeviceProvider ////

			/** initializes the driver. This will be called before any other methods are called. */
			virtual vr::EVRInitError Init(vr::IVRDriverContext* pDriverContext) override;

			/** cleans up the driver right before it is unloaded */
			virtual void Cleanup() override;

			/** Returns the version of the ITrackedDeviceServerDriver interface used by this driver */
			virtual const char* const* GetInterfaceVersions()
			{
				return vr::k_InterfaceVersions;
			}

			/** Allows the driver do to some work in the main loop of the server. Call frequency seems to be around 90Hz. */
			virtual void RunFrame() override;

			/** Returns true if the driver wants to block Standby mode. */
			virtual bool ShouldBlockStandbyMode() override
			{
				return false;
			}

			/** Called when the system is entering Standby mode */
			virtual void EnterStandby() override
			{
			}

			/** Called when the system is leaving Standby mode */
			virtual void LeaveStandby() override
			{
			}

			 //// self ////
			static ServerDriver* getInstance()
			{
				return singleton;
			}

			static std::string getInstallDirectory()
			{
				return installDir;
			}

			DeviceManipulationHandle* getDeviceManipulationHandleById(uint32_t unWhichDevice);

			// internal API

			/* Motion Compensation related */
			MotionCompensationManager& motionCompensation()
			{
				return m_motionCompensation;
			}
			void sendReplySetMotionCompensationMode(bool success);

			//// function hooks related ////
			void hooksTrackedDeviceAdded(void* serverDriverHost, int version, const char* pchDeviceSerialNumber, vr::ETrackedDeviceClass& eDeviceClass, void* pDriver);
			void hooksTrackedDeviceActivated(void* serverDriver, int version, uint32_t unObjectId);
			bool hooksTrackedDevicePoseUpdated(void* serverDriverHost, int version, uint32_t& unWhichDevice, vr::DriverPose_t& newPose, uint32_t& unPoseStructSize);

		private:
			static ServerDriver* singleton;

			static std::string installDir;			

			//// ipc shm related ////
			IpcShmCommunicator shmCommunicator;

			//// device manipulation related ////
			std::recursive_mutex _deviceManipulationHandlesMutex;
			std::map<void*, std::shared_ptr<DeviceManipulationHandle>> _deviceManipulationHandles;
			DeviceManipulationHandle* _openvrIdToDeviceManipulationHandleMap[vr::k_unMaxTrackedDeviceCount];
			std::map<void*, DeviceManipulationHandle*> _ptrToDeviceManipulationHandleMap;
			std::map<uint64_t, DeviceManipulationHandle*> _inputComponentToDeviceManipulationHandleMap;

			//// motion compensation related ////
			MotionCompensationManager m_motionCompensation;

			//// function hooks related ////
			std::shared_ptr<InterfaceHooks> _driverContextHooks;
		};
	} // end namespace driver
} // end namespace vrmotioncompensation