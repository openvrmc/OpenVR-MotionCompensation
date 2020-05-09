#include "DeviceManipulationTabController.h"
#include <QQuickWindow>
#include <QApplication>
#include "../overlaycontroller.h"
#include <openvr_math.h>
#include <ipc_protocol.h>
#include <chrono>

// application namespace
namespace motioncompensation
{
	DeviceManipulationTabController::~DeviceManipulationTabController()
	{
		if (identifyThread.joinable())
		{
			identifyThread.join();
		}
	}

	void DeviceManipulationTabController::initStage1()
	{
		reloadMotionCompensationSettings();
	}

	void DeviceManipulationTabController::initStage2(OverlayController* parent, QQuickWindow* widget)
	{
		this->parent = parent;
		this->widget = widget;

		//Fill the array with default data
		for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
		{
			deviceInfos.push_back(std::make_shared<DeviceInfo>());
		}

		LOG(DEBUG) << "deviceInfos size: " << deviceInfos.size();

		SearchDevices();
	}

	void DeviceManipulationTabController::eventLoopTick(vr::TrackedDevicePose_t* devicePoses)
	{
		if (settingsUpdateCounter >= 50)
		{
			settingsUpdateCounter = 0;			

			if (parent->isDashboardVisible() || parent->isDesktopMode())
			{
				unsigned i = 0;
				for (auto info : deviceInfos)
				{
					//Has the device mode changed?
					bool hasDeviceInfoChanged = updateDeviceInfo(i);

					//Has the connection status changed?
					unsigned status = devicePoses[info->openvrId].bDeviceIsConnected ? 0 : 1;
					if (info->deviceMode == vrmotioncompensation::MotionCompensationDeviceMode::Default && info->deviceStatus != status)
					{
						info->deviceStatus = status;
						hasDeviceInfoChanged = true;
					}

					//Push changes to UI
					if (hasDeviceInfoChanged)
					{
						emit deviceInfoChanged(i);
					}

					++i;
				}

				SearchDevices();
			}
		}
		else
		{
			settingsUpdateCounter++;
		}
	}

	bool DeviceManipulationTabController::SearchDevices()
	{
		bool newDeviceAdded = false;

		try
		{
			// Get some infos about the found devices
			for (uint32_t id = 0; id < vr::k_unMaxTrackedDeviceCount; ++id)
			{
				auto deviceClass = vr::VRSystem()->GetTrackedDeviceClass(id);

				if (deviceClass != vr::TrackedDeviceClass_Invalid && deviceInfos[id]->deviceClass == vr::TrackedDeviceClass_Invalid)
				{
					if (deviceClass == vr::TrackedDeviceClass_HMD || deviceClass == vr::TrackedDeviceClass_Controller || deviceClass == vr::TrackedDeviceClass_GenericTracker)
					{
						auto info = std::make_shared<DeviceInfo>();
						info->openvrId = id;
						info->deviceClass = deviceClass;
						char buffer[vr::k_unMaxPropertyStringSize];

						//Get and save the serial number
						vr::ETrackedPropertyError pError = vr::TrackedProp_Success;
						vr::VRSystem()->GetStringTrackedDeviceProperty(id, vr::Prop_SerialNumber_String, buffer, vr::k_unMaxPropertyStringSize, &pError);
						if (pError == vr::TrackedProp_Success)
						{
							info->serial = std::string(buffer);
						}
						else
						{
							info->serial = std::string("<unknown serial>");
							LOG(ERROR) << "Could not get serial of device " << id;
						}

						//Get and save the current device mode
						try
						{
							vrmotioncompensation::DeviceInfo info2;
							parent->vrMotionCompensation().getDeviceInfo(info->openvrId, info2);
							info->deviceMode = info2.deviceMode;
						}
						catch (std::exception& e)
						{
							LOG(ERROR) << "Exception caught while getting device info: " << e.what();
						}

						//Store the found info
						deviceInfos[id] = info;
						LOG(INFO) << "Found device: id " << info->openvrId << ", class " << info->deviceClass << ", serial " << info->serial;

						newDeviceAdded = true;
					}
				}
			}

			if (newDeviceAdded)
			{
				// Remove all map entries
				TrackerArrayIdToDeviceId.clear();
				HMDArrayIdToDeviceId.clear();
				emit deviceCountChanged();
			}
		}
		catch (const std::exception& e)
		{
			LOG(ERROR) << "Could not get device infos: " << e.what();
		}

		return newDeviceAdded;
	}

	void DeviceManipulationTabController::handleEvent(const vr::VREvent_t&)
	{

	}

	unsigned  DeviceManipulationTabController::getDeviceCount()
	{
		return (unsigned)deviceInfos.size();
	}

	QString DeviceManipulationTabController::getDeviceSerial(unsigned index)
	{
		if (index < deviceInfos.size())
		{
			return QString::fromStdString(deviceInfos[index]->serial);
		}
		else
		{
			return QString("<ERROR>");
		}
	}

	unsigned DeviceManipulationTabController::getOpenVRId(unsigned index)
	{
		if (index < deviceInfos.size())
		{
			return (int)deviceInfos[index]->openvrId;
		}
		else
		{
			return vr::k_unTrackedDeviceIndexInvalid;
		}
	}

	int DeviceManipulationTabController::getDeviceClass(unsigned index)
	{
		if (index < deviceInfos.size())
		{
			return (int)deviceInfos[index]->deviceClass;
		}
		else
		{
			return -1;
		}
	}

	int DeviceManipulationTabController::getDeviceState(unsigned index)
	{
		if (index < deviceInfos.size())
		{
			return (int)deviceInfos[index]->deviceStatus;
		}
		else
		{
			return -1;
		}
	}

	int DeviceManipulationTabController::getDeviceMode(unsigned index)
	{
		if (index < deviceInfos.size())
		{
			return (int)deviceInfos[index]->deviceMode;
		}
		else
		{
			return -1;
		}
	}

	double DeviceManipulationTabController::getLPFBeta()
	{
		return LPFBeta;
	}

	unsigned DeviceManipulationTabController::getSamples()
	{
		return samples;
	}

	void DeviceManipulationTabController::setTrackerArrayID(unsigned OpenVRId, unsigned ArrayID)
	{
		TrackerArrayIdToDeviceId.insert(std::make_pair(ArrayID, OpenVRId));
		LOG(DEBUG) << "Set Tracker Array ID, OpenVR ID: " << OpenVRId << ", Array ID: " << ArrayID;
	}

	void DeviceManipulationTabController::setHMDArrayID(unsigned OpenVRId, unsigned ArrayID)
	{
		HMDArrayIdToDeviceId.insert(std::make_pair(ArrayID, OpenVRId));
		LOG(DEBUG) << "Set HMD Array ID, OpenVR ID: " << OpenVRId << ", Array ID: " << ArrayID;
	}

	int DeviceManipulationTabController::getTrackerDeviceID(unsigned ArrayID)
	{
		//Search for the device ID
		auto search = TrackerArrayIdToDeviceId.find(ArrayID);
		if (search != TrackerArrayIdToDeviceId.end())
		{
			return search->second;
		}
		
		return -1;
	}

	int DeviceManipulationTabController::getHMDDeviceID(unsigned ArrayID)
	{
		//Search for the device ID
		auto search = HMDArrayIdToDeviceId.find(ArrayID);
		if (search != HMDArrayIdToDeviceId.end())
		{
			return search->second;
		}

		return -1;
	}

	void DeviceManipulationTabController::increaseLPFBeta(double value)
	{
		LPFBeta += value;

		if (LPFBeta > 1.0)
		{
			LPFBeta = 1.0;
		}
		else if (LPFBeta < 0.0)
		{
			LPFBeta = 0.0;
		}

		emit settingChanged();
	}

	void DeviceManipulationTabController::increaseSamples(int value)
	{
		samples += value;

		if (samples <= 2)
		{
			samples = 2;
		}

		emit settingChanged();
	}

	void DeviceManipulationTabController::reloadMotionCompensationSettings()
	{
		auto settings = OverlayController::appSettings();
		settings->beginGroup("deviceManipulationSettings");
		LPFBeta = settings->value("motionCompensationLPFBeta", 0.2).toDouble();
		samples = settings->value("motionCompensationSamples", 100).toUInt();
		settings->endGroup();

		LOG(INFO) << "Loading Data; LPF Beta: " << LPFBeta << "; samples: " << samples;
	}

	void DeviceManipulationTabController::saveMotionCompensationSettings()
	{
		LOG(INFO) << "Saving Data; LPF Beta: " << LPFBeta << ", samples: " << samples;
		auto settings = OverlayController::appSettings();
		settings->beginGroup("deviceManipulationSettings");
		settings->setValue("motionCompensationLPFBeta", LPFBeta);
		settings->setValue("motionCompensationSamples", samples);
		settings->endGroup();
		settings->sync();
	}

	// Enables or disables the motion compensation for the selected device
	bool DeviceManipulationTabController::setMotionCompensationMode(unsigned MCindex, unsigned RTindex, bool EnableMotionCompensation, bool setZero)
	{
		setZeroMode = setZero;

		// A few checks if the user input is valid
		if (MCindex < 0)
		{
			m_deviceModeErrorString = "Please select a device";
			return false;
		}

		if (RTindex < 0)
		{
			m_deviceModeErrorString = "Please select a reference tracker";
			return false;
		}

		//Search for the device ID
		auto search = TrackerArrayIdToDeviceId.find(RTindex);
		if (search != TrackerArrayIdToDeviceId.end())
		{
			RTindex = search->second;
		}
		else
		{
			m_deviceModeErrorString = "Invalid internal reference for RT";
			return false;
		}

		search = HMDArrayIdToDeviceId.find(MCindex);
		if (search != HMDArrayIdToDeviceId.end())
		{
			MCindex = search->second;
		}
		else
		{
			m_deviceModeErrorString = "Invalid internal reference for MC";
			return false;
		}

		LOG(DEBUG) << "Got these internal array IDs: HMD: " << MCindex << ", RTindex: " << RTindex;

		if (MCindex == RTindex)
		{
			m_deviceModeErrorString = "\"Device\" and \"Reference Tracker\" cannot be the same!";
			return false;
		}

		if (deviceInfos[MCindex]->deviceClass != vr::ETrackedDeviceClass::TrackedDeviceClass_HMD)
		{
			m_deviceModeErrorString = "\"Device\" is not a HMD!";
			return false;
		}

		if (deviceInfos[RTindex]->deviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_HMD)
		{
			m_deviceModeErrorString = "\"Reference Tracker\" cannot be a HMD!";
			return false;
		}

		if (deviceInfos[RTindex]->deviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_Invalid)
		{
			m_deviceModeErrorString = "\"Reference Tracker\" is invalid!";
			return false;
		}

		try
		{
			//Send new settings to the driver.dll
			if (EnableMotionCompensation)
			{
				LOG(INFO) << "Sending Motion Compensation Mode";
				motionCompensationMode = vrmotioncompensation::MotionCompensationMode::ReferenceTracker;
			}
			else
			{
				LOG(INFO) << "Sending Normal Mode";
				motionCompensationMode = vrmotioncompensation::MotionCompensationMode::Disabled;				
			}

			parent->vrMotionCompensation().setDeviceMotionCompensationMode(deviceInfos[MCindex]->openvrId, deviceInfos[RTindex]->openvrId, motionCompensationMode);
		}
		catch (vrmotioncompensation::vrmotioncompensation_exception & e)
		{
			switch (e.errorcode)
			{
				case (int)vrmotioncompensation::ipc::ReplyStatus::Ok:
				{
					m_deviceModeErrorString = "Not an error";
				} break;
				case (int)vrmotioncompensation::ipc::ReplyStatus::InvalidId:
				{
					m_deviceModeErrorString = "Invalid Id";
				} break;
				case (int)vrmotioncompensation::ipc::ReplyStatus::NotFound:
				{
					m_deviceModeErrorString = "Device not found";
				} break;
				default:
				{
					m_deviceModeErrorString = "SteamVR did not load OVRMC .dll";
				} break;
			}
			LOG(ERROR) << "Exception caught while setting device mode: " << e.what();

			return false;
		}
		catch (std::exception & e)
		{
			m_deviceModeErrorString = "Unknown exception";
			LOG(ERROR) << "Unknown exception caught while setting device mode: " << e.what();

			return false;
		}

		saveMotionCompensationSettings();

		/*if (notify)
		{
			updateDeviceInfo(index);
			emit deviceInfoChanged(index);
		}*/

		return true;
	}

	bool DeviceManipulationTabController::sendMCSettings()
	{
		try
		{
			LOG(INFO) << "Sending Motion Compensation settings. LPF Beta: " << LPFBeta << "; samples: " << samples << "; ZeroMode: " << setZeroMode;
			parent->vrMotionCompensation().setMoticonCompensationSettings(LPFBeta, samples, setZeroMode);
		}
		catch (vrmotioncompensation::vrmotioncompensation_exception& e)
		{
			switch (e.errorcode)
			{
				case (int)vrmotioncompensation::ipc::ReplyStatus::Ok:
				{
					m_deviceModeErrorString = "Not an error";					
				} break;
				default:
				{
					m_deviceModeErrorString = "SteamVR did not load OVRMC .dll";
				} break;

				LOG(ERROR) << "Exception caught while setting LPF Beta: " << e.what();

				return false;
			}
		}
		catch (std::exception& e)
		{
			m_deviceModeErrorString = "Unknown exception";
			LOG(ERROR) << "Exception caught while setting LPF Beta: " << e.what();

			return false;
		}

		return true;
	}

	bool DeviceManipulationTabController::setLPFBeta(double value)
	{
		// A few checks if the user input is valid
		if (value <= 0.0)
		{
			m_deviceModeErrorString = "Value cannot be lower than 0.0001";
			return false;
		}
		if (value > 1.0)
		{
			m_deviceModeErrorString = "Value cannot be higher than 1.0000";
			return false;
		}

		LPFBeta = value;		

		return true;
	}

	bool DeviceManipulationTabController::setSamples(unsigned value)
	{
		// A few checks if the user input is valid
		if (value < 1)
		{
			m_deviceModeErrorString = "Samples cannot be lower than 1";
			return false;
		}

		samples = value;

		return true;
	}

	bool DeviceManipulationTabController::setDebugMode(bool TestForStandby)
	{
		bool enable = false;
		QString newButtonText = "";
		int newLoggerStatus = 0;

		// Queue new debug logger state
		if (TestForStandby && motionCompensationMode == vrmotioncompensation::MotionCompensationMode::ReferenceTracker && DebugLoggerStatus == 1)
		{
			enable = true;
			newLoggerStatus = 2;
			newButtonText = "Stop logging";
		}
		else if (TestForStandby)
		{
			// return from function if standby mode was not active
			return true;
		}
		else if (!TestForStandby && motionCompensationMode == vrmotioncompensation::MotionCompensationMode::Disabled && DebugLoggerStatus == 0)
		{
			newLoggerStatus = 1;
			newButtonText = "Standby...";
		}
		else if ((!TestForStandby && motionCompensationMode == vrmotioncompensation::MotionCompensationMode::Disabled && DebugLoggerStatus == 1) ||
				 (!TestForStandby && motionCompensationMode == vrmotioncompensation::MotionCompensationMode::ReferenceTracker && DebugLoggerStatus == 2))
		{
			enable = false;
			newLoggerStatus = 0;
			newButtonText = "Start logging";
		}
		else if (!TestForStandby && motionCompensationMode == vrmotioncompensation::MotionCompensationMode::ReferenceTracker && DebugLoggerStatus == 0)
		{
			enable = true;
			newLoggerStatus = 2;
			newButtonText = "Stop logging";
		}

		// Only send new state when logger is not in standby mode
		if (newLoggerStatus != 1)
		{
			try
			{
				LOG(INFO) << "Sending debug mode (Status: " << newLoggerStatus << ")";
				parent->vrMotionCompensation().startDebugLogger(enable);
			}
			catch (vrmotioncompensation::vrmotioncompensation_exception& e)
			{
				switch (e.errorcode)
				{
					case (int)vrmotioncompensation::ipc::ReplyStatus::Ok:
					{
						m_deviceModeErrorString = "Not an error";
					} break;
					case (int)vrmotioncompensation::ipc::ReplyStatus::InvalidId:
					{
						m_deviceModeErrorString = "MC must be running to\nstart the debug logger";
					} break;
					default:
					{
						m_deviceModeErrorString = "Unknown error";
					} break;

					LOG(ERROR) << "Exception caught while sending debug mode: " << e.what();

					return false;
				}
			}
			catch (std::exception& e)
			{
				m_deviceModeErrorString = "Unknown exception";
				LOG(ERROR) << "Exception caught while sending debug mode: " << e.what();

				return false;
			}
		}

		// If send was successful or not needed, apply state
		DebugLoggerStatus = newLoggerStatus;
		debugModeButtonString = newButtonText;

		emit debugModeChanged();

		return true;
	}

	QString DeviceManipulationTabController::getDebugModeButtonText()
	{
		return debugModeButtonString;
	}

	QString DeviceManipulationTabController::getDeviceModeErrorString()
	{
		return m_deviceModeErrorString;
	}

	bool DeviceManipulationTabController::updateDeviceInfo(unsigned OpenVRId)
	{
		bool retval = false;

		if (OpenVRId < deviceInfos.size())
		{
			try
			{
				vrmotioncompensation::DeviceInfo info;

				parent->vrMotionCompensation().getDeviceInfo(deviceInfos[OpenVRId]->openvrId, info);
				if (deviceInfos[OpenVRId]->deviceMode != info.deviceMode)
				{
					deviceInfos[OpenVRId]->deviceMode = info.deviceMode;
					retval = true;
				}
			}
			catch (std::exception& e)
			{
				LOG(ERROR) << "Exception caught while getting device info: " << e.what();
			}
		}

		return retval;
	}
} // namespace motioncompensation