#include "ServerDriver.h"
#include "../devicemanipulation/DeviceManipulationHandle.h"
#include <set>
#include <mutex>

namespace vrmotioncompensation
{
	namespace driver
	{

		ServerDriver* ServerDriver::singleton = nullptr;
		std::string ServerDriver::installDir;

		// Use shared_ptr to prevent use-after-free
		std::shared_ptr<DeviceManipulationHandle> ServerDriver::_openvrIdDeviceManipulationHandle[vr::k_unMaxTrackedDeviceCount];

		ServerDriver::ServerDriver() : m_motionCompensation(this)
		{
			singleton = this;
			// No need to memset shared_ptr array — it's zero-initialized in global scope
			// But we ensure it here for clarity
			std::fill(std::begin(_openvrIdDeviceManipulationHandle), std::end(_openvrIdDeviceManipulationHandle), nullptr);
			std::fill(std::begin(_deviceVersionMap), std::end(_deviceVersionMap), 0);
		}

		ServerDriver::~ServerDriver()
		{
			LOG(TRACE) << "driver::~ServerDriver()";
			singleton = nullptr;
		}

		// === POSE UPDATE ===
		// Called ~700–1000 Hz — must be fast and thread-safe
		bool ServerDriver::hooksTrackedDevicePoseUpdated(void* serverDriverHost, int version,
			uint32_t& unWhichDevice, vr::DriverPose_t& newPose, uint32_t& unPoseStructSize)
		{
			if (unWhichDevice >= vr::k_unMaxTrackedDeviceCount)
				return true;

			auto handle = _openvrIdDeviceManipulationHandle[unWhichDevice];

			if (!handle)
			{
				static std::set<uint32_t> attemptedRegistration;
				static std::mutex registrationMutex;

				{
					std::lock_guard<std::mutex> lock(registrationMutex);
					if (attemptedRegistration.find(unWhichDevice) != attemptedRegistration.end())
					{
						return true;
					}
					attemptedRegistration.insert(unWhichDevice);
				}

				std::lock_guard<std::recursive_mutex> lock(_deviceManipulationHandlesMutex);

				handle = _openvrIdDeviceManipulationHandle[unWhichDevice];
				if (!handle)
				{
					vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(unWhichDevice);
					if (container != vr::k_ulInvalidPropertyContainer)
					{
						char serial[1024] = { 0 };
						vr::ETrackedPropertyError err;
						vr::VRProperties()->GetStringProperty(container, vr::Prop_SerialNumber_String, serial, sizeof(serial), &err);

						if (err == vr::TrackedProp_Success && serial[0] != '\0')
						{
							int32_t deviceClass = vr::VRProperties()->GetInt32Property(container, vr::Prop_DeviceClass_Int32, &err);
							if (err != vr::TrackedProp_Success)
								deviceClass = vr::TrackedDeviceClass_Invalid;

							LOG(WARNING) << "LAZY REGISTRATION: Device " << serial
								<< " (class: " << deviceClass << ", ID: " << unWhichDevice
								<< ") was added before our driver initialized!";

							handle = std::make_shared<DeviceManipulationHandle>(serial, (vr::ETrackedDeviceClass)deviceClass);
							handle->setOpenvrId(unWhichDevice);
							_openvrIdDeviceManipulationHandle[unWhichDevice] = handle;


							LOG(INFO) << "Successfully lazy-registered device: " << serial;
						}
						else
						{
							LOG(ERROR) << "Could not get serial for device ID " << unWhichDevice;
						}
					}
					else
					{
						LOG(ERROR) << "Could not get property container for device ID " << unWhichDevice;
					}
				}
			}

			if (handle && handle->isValid())
			{
				return handle->handlePoseUpdate(unWhichDevice, newPose, unPoseStructSize);
			}

			return true;
		}

		// === DEVICE ADDED ===
		void ServerDriver::hooksTrackedDeviceAdded(void* serverDriverHost, int version,
			const char* pchDeviceSerialNumber, vr::ETrackedDeviceClass eDeviceClass, void* pDriver)
		{
			LOG(TRACE) << "hooksTrackedDeviceAdded: " << (pchDeviceSerialNumber ? pchDeviceSerialNumber : "null")
				<< " (class: " << (int)eDeviceClass << ")";

			if (!pDriver || !pchDeviceSerialNumber || pchDeviceSerialNumber[0] == '\0')
				return;

			auto handle = std::make_shared<DeviceManipulationHandle>(pchDeviceSerialNumber, eDeviceClass);

			{
				std::lock_guard<std::recursive_mutex> lock(_deviceManipulationHandlesMutex);
				_deviceManipulationHandles[pDriver] = handle;
			}

			LOG(INFO) << "Pre-registered device: " << pchDeviceSerialNumber;

		}

		// === DEVICE ACTIVATED (gets OpenVR ID) ===
		void ServerDriver::hooksTrackedDeviceActivated(void* serverDriver, int version, uint32_t unObjectId)
		{
			if (unObjectId >= vr::k_unMaxTrackedDeviceCount)
				return;

			std::lock_guard<std::recursive_mutex> lock(_deviceManipulationHandlesMutex);

			auto it = _deviceManipulationHandles.find(serverDriver);
			if (it != _deviceManipulationHandles.end())
			{
				auto handle = it->second;
				handle->setOpenvrId(unObjectId);

				_openvrIdDeviceManipulationHandle[unObjectId] = handle;

				std::shared_ptr<InterfaceHooks> hookedInterface;
				const char* versions[] = {
					"ITrackedDeviceServerDriver_005",
					"ITrackedDeviceServerDriver_006",
					"ITrackedDeviceServerDriver_007"
				};

				for (const char* ver : versions) {
					hookedInterface = InterfaceHooks::hookInterface(serverDriver, ver);
					if (hookedInterface) {
						LOG(INFO) << "Hooked " << ver << " for " << handle->serialNumber()
							<< " (ID " << unObjectId << ")";
						handle->setServerDriverHooks(hookedInterface);
						break;
					}
				}

				if (!hookedInterface) {
					LOG(WARNING) << "Failed to hook interface for " << handle->serialNumber()
						<< " (ID " << unObjectId << ")";
				}

				LOG(INFO) << "Activated device: " << handle->serialNumber()
					<< " -> OpenVR ID: " << unObjectId;
			}
			else
			{
				LOG(WARNING) << "Activated unknown device ID: " << unObjectId;
			}
		}

		// === DEVICE DEACTIVATED ===
		void ServerDriver::hooksTrackedDeviceDeactivated(void* serverDriver, int version, uint32_t unObjectId)
		{
			if (unObjectId >= vr::k_unMaxTrackedDeviceCount)
				return;

			LOG(INFO) << "Device deactivated: OpenVR ID " << unObjectId;

			// Clear from array using shared_ptr (safe even if used concurrently)
			_openvrIdDeviceManipulationHandle[unObjectId].reset();

			// Optional: clear from map if you want (not strictly needed — will be cleaned on removal)
		}

		// === DEVICE REMOVED ===
		void ServerDriver::hooksTrackedDeviceRemoved(void* serverDriver, int version, uint32_t unObjectId)
		{
			if (unObjectId < vr::k_unMaxTrackedDeviceCount)
			{
				_openvrIdDeviceManipulationHandle[unObjectId].reset();
			}

			std::lock_guard<std::recursive_mutex> lock(_deviceManipulationHandlesMutex);
			auto it = _deviceManipulationHandles.find(serverDriver);
			if (it != _deviceManipulationHandles.end())
			{
				LOG(INFO) << "Removed device: " << it->second->serialNumber();
				_deviceManipulationHandles.erase(it);
			}
		}

		// === INIT ===
		vr::EVRInitError ServerDriver::Init(vr::IVRDriverContext* pDriverContext)
		{
			LOG(INFO) << "ServerDriver::Init()";

			InterfaceHooks::setServerDriver(this);

			auto mhError = MH_Initialize();
			if (mhError != MH_OK) {
				LOG(ERROR) << "MinHook init failed: " << MH_StatusToString(mhError);
				return vr::VRInitError_Driver_Failed;
			}

			_driverContextHooks = InterfaceHooks::hookInterface(pDriverContext, "IVRDriverContext");
			VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);

			vr::ETrackedPropertyError propError = vr::TrackedProp_Success;
			installDir = vr::VRProperties()->GetStringProperty(
				pDriverContext->GetDriverHandle(),
				vr::Prop_InstallPath_String, &propError);

			if (propError == vr::TrackedProp_Success)
				LOG(INFO) << "Driver install directory: " << installDir;

			shmCommunicator.init(this);

			return vr::VRInitError_None;
		}

		// === CLEANUP ===
		void ServerDriver::Cleanup()
		{
			LOG(TRACE) << "ServerDriver::Cleanup()";

			shmCommunicator.shutdown();

			{
				std::lock_guard<std::recursive_mutex> lock(_deviceManipulationHandlesMutex);
				_deviceManipulationHandles.clear();
			}

			// Clear all tracked devices
			for (auto& ptr : _openvrIdDeviceManipulationHandle)
				ptr.reset();

			_driverContextHooks.reset();
			MH_Uninitialize();
			VR_CLEANUP_SERVER_DRIVER_CONTEXT();
		}

		// === RUNFRAME (optional) ===
		void ServerDriver::RunFrame()
		{
			// You can do lightweight per-frame work here if needed
			// shmCommunicator is already running in background thread
		}

		// === PUBLIC ACCESSOR ===
		DeviceManipulationHandle* ServerDriver::getDeviceManipulationHandleById(uint32_t unWhichDevice)
		{
			if (unWhichDevice >= vr::k_unMaxTrackedDeviceCount)
				return nullptr;

			auto handle = _openvrIdDeviceManipulationHandle[unWhichDevice];
			if (handle && handle->isValid())
				return handle.get();

			return nullptr;
		}

	} // namespace driver
} // namespace vrmotioncompensation