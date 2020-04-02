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
		reloadDeviceManipulationProfiles();
		reloadMotionCompensationSettings();
	}

	void DeviceManipulationTabController::initStage2(OverlayController* parent, QQuickWindow* widget)
	{
		this->parent = parent;
		this->widget = widget;

		SearchDevices(0);
		/*try
		{
			//Get some infos about the found devices
			for (uint32_t id = 0; id < vr::k_unMaxTrackedDeviceCount; ++id)
			{
				auto deviceClass = vr::VRSystem()->GetTrackedDeviceClass(id);
				if (deviceClass != vr::TrackedDeviceClass_Invalid)
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
						deviceInfos.push_back(info);
						LOG(INFO) << "Found device: id " << info->openvrId << ", class " << info->deviceClass << ", serial " << info->serial;
					}
					maxValidDeviceId = id;
				}
			}

			emit deviceCountChanged((unsigned)deviceInfos.size());
		}
		catch (const std::exception & e)
		{
			LOG(ERROR) << "Could not get device infos: " << e.what();
		}*/
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

				SearchDevices(maxValidDeviceId + 1);

				//Check if there is a new device (id start point is maxValidDeviceId + 1!)
				/*bool newDeviceAdded = false;
				for (uint32_t id = maxValidDeviceId + 1; id < vr::k_unMaxTrackedDeviceCount; ++id)
				{
					auto deviceClass = vr::VRSystem()->GetTrackedDeviceClass(id);
					if (deviceClass != vr::TrackedDeviceClass_Invalid)
					{
						if (deviceClass == vr::TrackedDeviceClass_Controller || deviceClass == vr::TrackedDeviceClass_GenericTracker)
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

							deviceInfos.push_back(info);
							LOG(INFO) << "Found device: id " << info->openvrId << ", class " << info->deviceClass << ", serial " << info->serial;
							newDeviceAdded = true;
						}
						maxValidDeviceId = id;
					}
				}
				if (newDeviceAdded)
				{
					emit deviceCountChanged((unsigned)deviceInfos.size());
				}*/
			}
		}
		else
		{
			settingsUpdateCounter++;
		}
	}

	bool DeviceManipulationTabController::SearchDevices(int StartID)
	{
		bool newDeviceAdded = false;

		try
		{
			//Get some infos about the found devices
			for (uint32_t id = StartID; id < vr::k_unMaxTrackedDeviceCount; ++id)
			{
				auto deviceClass = vr::VRSystem()->GetTrackedDeviceClass(id);
				if (deviceClass != vr::TrackedDeviceClass_Invalid)
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
						deviceInfos.push_back(info);
						LOG(INFO) << "Found device: id " << info->openvrId << ", class " << info->deviceClass << ", serial " << info->serial;

						newDeviceAdded = true;
					}

					maxValidDeviceId = id;
				}
			}

			if (newDeviceAdded)
			{
				emit deviceCountChanged((unsigned)deviceInfos.size());
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
		/*switch (vrEvent.eventType) {
			default:
				break;
		}*/
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

	void DeviceManipulationTabController::setTrackerArrayID(unsigned deviceID, unsigned ArrayID)
	{
		TrackerArrayIdToDeviceId.insert(std::make_pair(ArrayID, deviceID));
		LOG(DEBUG) << "Set Tracker Array ID, device ID: " << deviceID << " Array ID: " << ArrayID;
	}

	void DeviceManipulationTabController::setHMDArrayID(unsigned deviceID, unsigned ArrayID)
	{
		HMDArrayIdToDeviceId.insert(std::make_pair(ArrayID, deviceID));
		LOG(DEBUG) << "Set HMD Array ID, device ID: " << deviceID << " Array ID: " << ArrayID;
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
	}

	void DeviceManipulationTabController::reloadMotionCompensationSettings()
	{
		auto settings = OverlayController::appSettings();
		settings->beginGroup("deviceManipulationSettings");
		motionCompensationMode = (vrmotioncompensation::MotionCompensationMode)settings->value("motionCompensationVelAccMode", 0).toUInt();
		LPFBeta = settings->value("motionCompensationLPFBeta", 0.2).toDouble();
		settings->endGroup();
	}

	void DeviceManipulationTabController::reloadDeviceManipulationProfiles()
	{
		deviceManipulationProfiles.clear();
		auto settings = OverlayController::appSettings();
		settings->beginGroup("deviceManipulationSettings");
		auto profileCount = settings->beginReadArray("deviceManipulationProfiles");
		for (int i = 0; i < profileCount; i++)
		{
			settings->setArrayIndex(i);
			deviceManipulationProfiles.emplace_back();
			auto& entry = deviceManipulationProfiles[i];
			entry.profileName = settings->value("profileName").toString().toStdString();
		}
		settings->endArray();
		settings->endGroup();
	}

	void DeviceManipulationTabController::saveMotionCompensationSettings()
	{
		auto settings = OverlayController::appSettings();
		settings->beginGroup("deviceManipulationSettings");
		settings->setValue("motionCompensationVelAccMode", (unsigned)motionCompensationMode);
		settings->setValue("motionCompensationLPFBeta", LPFBeta);
		settings->endGroup();
		settings->sync();
	}

	void DeviceManipulationTabController::saveDeviceManipulationProfiles()
	{
		auto settings = OverlayController::appSettings();
		settings->beginGroup("deviceManipulationSettings");
		settings->beginWriteArray("deviceManipulationProfiles");
		unsigned i = 0;

		for (auto& p : deviceManipulationProfiles)
		{
			settings->setArrayIndex(i);
			settings->setValue("profileName", QString::fromStdString(p.profileName));
		}
		settings->endArray();
		settings->endGroup();
		settings->sync();
	}

	unsigned DeviceManipulationTabController::getDeviceManipulationProfileCount()
	{
		return (unsigned)deviceManipulationProfiles.size();
	}

	QString DeviceManipulationTabController::getDeviceManipulationProfileName(unsigned index)
	{
		if (index >= deviceManipulationProfiles.size())
		{
			return QString();
		}
		else
		{
			return QString::fromStdString(deviceManipulationProfiles[index].profileName);
		}
	}

	void DeviceManipulationTabController::addDeviceManipulationProfile(QString name, unsigned deviceIndex, bool includesDeviceOffsets, bool includesInputRemapping)
	{
		if (deviceIndex >= deviceInfos.size())
		{
			return;
		}

		auto device = deviceInfos[deviceIndex];
		DeviceManipulationProfile* profile = nullptr;
		for (auto& p : deviceManipulationProfiles)
		{
			if (p.profileName.compare(name.toStdString()) == 0)
			{
				profile = &p;
				break;
			}
		}

		if (!profile)
		{
			auto i = deviceManipulationProfiles.size();
			deviceManipulationProfiles.emplace_back();
			profile = &deviceManipulationProfiles[i];
		}

		profile->profileName = name.toStdString();
		saveDeviceManipulationProfiles();
		OverlayController::appSettings()->sync();
		emit deviceManipulationProfilesChanged();
	}

	void DeviceManipulationTabController::applyDeviceManipulationProfile(unsigned index, unsigned deviceIndex)
	{
		if (index < deviceManipulationProfiles.size())
		{
			if (deviceIndex >= deviceInfos.size())
			{
				return;
			}
			auto device = deviceInfos[deviceIndex];
			auto& profile = deviceManipulationProfiles[index];
			emit deviceInfoChanged(deviceIndex);
		}
	}

	void DeviceManipulationTabController::deleteDeviceManipulationProfile(unsigned index)
	{
		if (index < deviceManipulationProfiles.size())
		{
			auto pos = deviceManipulationProfiles.begin() + index;
			deviceManipulationProfiles.erase(pos);
			saveDeviceManipulationProfiles();
			OverlayController::appSettings()->sync();
			emit deviceManipulationProfilesChanged();
		}
	}

	// Enables or disables the motion compensation for the selected device
	bool DeviceManipulationTabController::setMotionCompensationMode(unsigned MCindex, unsigned RTindex, bool EnableMotionCompensation/*, bool notify*/)
	{
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

		LOG(DEBUG) << "setMotionCompensationMode Array IDs, MCindex: " << MCindex << " RTindex: " << RTindex;

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

		LOG(DEBUG) << "setMotionCompensationMode device IDs, MCindex: " << MCindex << " RTindex: " << RTindex << " MaxValid ID: " << maxValidDeviceId;

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
				//parent->vrMotionCompensation().setDeviceMotionCompensationMode(deviceInfos[MCindex]->openvrId, deviceInfos[RTindex]->openvrId, motionCompensationMode);
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
					m_deviceModeErrorString = "Unknown error";
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

		/*if (notify)
		{
			updateDeviceInfo(index);
			emit deviceInfoChanged(index);
		}*/

		return true;
	}

	bool DeviceManipulationTabController::sendLPFBeta()
	{
		try
		{
			LOG(INFO) << "Sending LPF Beta value: " << LPFBeta;
			parent->vrMotionCompensation().setLPFBeta(LPFBeta);
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
					m_deviceModeErrorString = "Unknown error";					
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

	QString DeviceManipulationTabController::getDeviceModeErrorString()
	{
		return m_deviceModeErrorString;
	}

	bool DeviceManipulationTabController::updateDeviceInfo(unsigned index)
	{
		bool retval = false;

		/*if (index < deviceInfos.size())
		{
			try
			{
				vrmotioncompensation::DeviceInfo info;

				parent->vrMotionCompensation().getDeviceInfo(deviceInfos[index]->openvrId, info);
				if (deviceInfos[index]->deviceMode != info.deviceMode)
				{
					deviceInfos[index]->deviceMode = info.deviceMode;
					retval = true;
				}
			}
			catch (std::exception& e)
			{
				LOG(ERROR) << "Exception caught while getting device info: " << e.what();
			}
		}*/

		return retval;
	}
} // namespace motioncompensation