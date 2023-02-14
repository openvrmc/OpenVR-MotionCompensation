#include "ServerDriver.h"
#include "../devicemanipulation/DeviceManipulationHandle.h"

namespace vrmotioncompensation
{
	namespace driver
	{
		ServerDriver* ServerDriver::singleton = nullptr;
		std::string ServerDriver::installDir;

		ServerDriver::ServerDriver() : m_motionCompensation(this)
		{
			singleton = this;
			memset(_openvrIdDeviceManipulationHandle, 0, sizeof(DeviceManipulationHandle*) * vr::k_unMaxTrackedDeviceCount);  // Array of DeviceManipulationHandles. Set all of them to 0 at the start.
			memset(_deviceVersionMap, 0, sizeof(int) * vr::k_unMaxTrackedDeviceCount);
		}

		ServerDriver::~ServerDriver()
		{
			LOG(TRACE) << "driver::~ServerDriver()";
		}

		// This is called for every device that we are tracking. This calls the related DeviceManipulationHandle, which then handles the pose update according to device Mode (MC or RefTracker or nothing).
		bool ServerDriver::hooksTrackedDevicePoseUpdated(void* serverDriverHost, int version, uint32_t& unWhichDevice, vr::DriverPose_t& newPose, uint32_t& unPoseStructSize)
		{
			if (_openvrIdDeviceManipulationHandle[unWhichDevice] && _openvrIdDeviceManipulationHandle[unWhichDevice]->isValid())
			{
				if (_deviceVersionMap[unWhichDevice] == 0)
				{
					_deviceVersionMap[unWhichDevice] = version;
				}

				//LOG(TRACE) << "ServerDriver::hooksTrackedDevicePoseUpdated(version:" << version << ", deviceId:" << unWhichDevice << ", first used version: " << _deviceVersionMap[unWhichDevice] << ")";
				
				if (_deviceVersionMap[unWhichDevice] == version)
				{
					return _openvrIdDeviceManipulationHandle[unWhichDevice]->handlePoseUpdate(unWhichDevice, newPose, unPoseStructSize);
				}

				//LOG(TRACE) << "ServerDriver::hooksTrackedDevicePoseUpdated called for wrong version, ignoring ";
			}
			return true;
		}

		void ServerDriver::hooksTrackedDeviceAdded(void* serverDriverHost, int version, const char* pchDeviceSerialNumber, vr::ETrackedDeviceClass& eDeviceClass, void* pDriver)
		{
			LOG(TRACE) << "ServerDriver::hooksTrackedDeviceAdded(" << serverDriverHost << ", " << version << ", " << pchDeviceSerialNumber << ", " << (int)eDeviceClass << ", " << pDriver << ")";
			LOG(INFO) << "Found device " << pchDeviceSerialNumber << " (deviceClass: " << (int)eDeviceClass << ")";

			// Create ManipulationInfo entry
			auto handle = std::make_shared<DeviceManipulationHandle>(pchDeviceSerialNumber, eDeviceClass);
			_deviceManipulationHandles.insert({ pDriver, handle });

			// Hook into server driver interface
			handle->setServerDriverHooks(InterfaceHooks::hookInterface(pDriver, "ITrackedDeviceServerDriver_005"));
		}

		// THOMAS: Gets called by the driver when a device is "Activated", whatever that means. Here the handle information is completed, and can be passed back to the IPC message upon request.
		void ServerDriver::hooksTrackedDeviceActivated(void* serverDriver, int version, uint32_t unObjectId)
		{
			LOG(TRACE) << "ServerDriver::hooksTrackedDeviceActivated(" << serverDriver << ", " << version << ", " << unObjectId << ")";

			// Search for the activated device
			auto i = _deviceManipulationHandles.find(serverDriver);

			if (i != _deviceManipulationHandles.end())
			{
				auto handle = i->second;
				handle->setOpenvrId(unObjectId);
				_openvrIdDeviceManipulationHandle[unObjectId] = handle.get();

				//LOG(INFO) << "Successfully added device " << handle->serialNumber() << " (OpenVR Id: " << handle->openvrId() << ")";
				LOG(INFO) << "Successfully added device " << _openvrIdDeviceManipulationHandle[unObjectId]->serialNumber() << " (OpenVR Id: " << _openvrIdDeviceManipulationHandle[unObjectId]->openvrId() << ")";
			}
		}

		vr::EVRInitError ServerDriver::Init(vr::IVRDriverContext* pDriverContext)
		{
			LOG(INFO) << "CServerDriver::Init()";

			// Initialize Hooking
			InterfaceHooks::setServerDriver(this);
			auto mhError = MH_Initialize();
			if (mhError == MH_OK)
			{
				_driverContextHooks = InterfaceHooks::hookInterface(pDriverContext, "IVRDriverContext");
			}
			else
			{
				LOG(ERROR) << "Error while initializing minHook: " << MH_StatusToString(mhError);
			}

			LOG(DEBUG) << "Initialize driver context.";
			VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);

			// Read installation directory
			vr::ETrackedPropertyError tpeError;
			installDir = vr::VRProperties()->GetStringProperty(pDriverContext->GetDriverHandle(), vr::Prop_InstallPath_String, &tpeError);
			if (tpeError == vr::TrackedProp_Success)
			{
				LOG(INFO) << "Install Dir:" << installDir;
			}
			else
			{
				LOG(INFO) << "Could not get Install Dir: " << vr::VRPropertiesRaw()->GetPropErrorNameFromEnum(tpeError);
			}

			// Start IPC thread
			shmCommunicator.init(this);
			return vr::VRInitError_None;
		}

		void ServerDriver::Cleanup()
		{
			LOG(TRACE) << "ServerDriver::Cleanup()";
			_driverContextHooks.reset();
			MH_Uninitialize();
			shmCommunicator.shutdown();
			VR_CLEANUP_SERVER_DRIVER_CONTEXT();
		}

		// Call frequency: ~93Hz
		void ServerDriver::RunFrame()
		{

		}

		DeviceManipulationHandle* ServerDriver::getDeviceManipulationHandleById(uint32_t unWhichDevice)
		{
			LOG(TRACE) << "getDeviceByID: unWhichDevice: " << unWhichDevice;

			std::lock_guard<std::recursive_mutex> lock(_deviceManipulationHandlesMutex);

			if (_openvrIdDeviceManipulationHandle[unWhichDevice]->isValid())
			{
				if (_openvrIdDeviceManipulationHandle[unWhichDevice])
				{
					return _openvrIdDeviceManipulationHandle[unWhichDevice];
				}
				else
				{
					LOG(ERROR) << "_openvrIdDeviceManipulationHandle[unWhichDevice] is NULL. unWhichDevice: " << unWhichDevice;
				}
			}
			else
			{
				LOG(ERROR) << "_openvrIdDeviceManipulationHandle[unWhichDevice] is not valid. unWhichDevice: " << unWhichDevice;
			}

			return nullptr;
		}

	} // end namespace driver
} // end namespace vrmotioncompensation