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
			memset(_openvrIdToDeviceManipulationHandleMap, 0, sizeof(DeviceManipulationHandle*) * vr::k_unMaxTrackedDeviceCount);
		}

		ServerDriver::~ServerDriver()
		{
			LOG(TRACE) << "driver::~ServerDriver()";
		}

		bool ServerDriver::hooksTrackedDevicePoseUpdated(void* serverDriverHost, int version, uint32_t& unWhichDevice, vr::DriverPose_t& newPose, uint32_t& unPoseStructSize)
		{
			if (_openvrIdToDeviceManipulationHandleMap[unWhichDevice] && _openvrIdToDeviceManipulationHandleMap[unWhichDevice]->isValid())
			{
				return _openvrIdToDeviceManipulationHandleMap[unWhichDevice]->handlePoseUpdate(unWhichDevice, newPose, unPoseStructSize);
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

		void ServerDriver::hooksTrackedDeviceActivated(void* serverDriver, int version, uint32_t unObjectId)
		{
			LOG(TRACE) << "ServerDriver::hooksTrackedDeviceActivated(" << serverDriver << ", " << version << ", " << unObjectId << ")";
			
			//Search for the activated device
			auto i = _deviceManipulationHandles.find(serverDriver);

			if (i != _deviceManipulationHandles.end())
			{
				auto handle = i->second;
				handle->setOpenvrId(unObjectId);
				_openvrIdToDeviceManipulationHandleMap[unObjectId] = handle.get();

				LOG(INFO) << "Successfully added device " << handle->serialNumber() << " (OpenVR Id: " << handle->openvrId() << ")";
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
				LOG(ERROR) << "Error while initialising minHook: " << MH_StatusToString(mhError);
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
			m_motionCompensation.StopDebugData();

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

			if (_openvrIdToDeviceManipulationHandleMap[unWhichDevice] && _openvrIdToDeviceManipulationHandleMap[unWhichDevice]->isValid())
			{
				return _openvrIdToDeviceManipulationHandleMap[unWhichDevice];
			}

			return nullptr;
		}

		void ServerDriver::sendReplySetMotionCompensationMode(bool success)
		{
			shmCommunicator.sendReplySetMotionCompensationMode(success);
		}

	} // end namespace driver
} // end namespace vrmotioncompensation