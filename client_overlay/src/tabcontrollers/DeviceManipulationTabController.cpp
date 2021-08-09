#include "DeviceManipulationTabController.h"
#include <QQuickWindow>
#include <QApplication>
#include "../overlaycontroller.h"
#include <openvr_math.h>
#include <ipc_protocol.h>
#include <chrono>
#include <QQmlProperty>

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
		InitShortcuts();
		reloadMotionCompensationSettings();		
	}

	void DeviceManipulationTabController::Beenden()
	{
		qApp->quit();
	}

	void DeviceManipulationTabController::initStage2(OverlayController* parent, QQuickWindow* widget)
	{
		this->parent = parent;
		this->widget = widget;

		// Fill the array with default data
		for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
		{
			deviceInfos.push_back(std::make_shared<DeviceInfo>());
		}

		LOG(DEBUG) << "deviceInfos size: " << deviceInfos.size();

		SearchDevices();

		parent->vrMotionCompensation().setOffsets(_offset);
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
					// Has the device mode changed?
					bool hasDeviceInfoChanged = updateDeviceInfo(i);

					// Has the connection status changed?
					unsigned status = devicePoses[info->openvrId].bDeviceIsConnected ? 0 : 1;
					if (info->deviceMode == vrmotioncompensation::MotionCompensationDeviceMode::Default && info->deviceStatus != status)
					{
						info->deviceStatus = status;
						hasDeviceInfoChanged = true;
					}

					// Push changes to UI
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
				vr::ETrackedDeviceClass deviceClass = vr::VRSystem()->GetTrackedDeviceClass(id);

				if (deviceClass != vr::TrackedDeviceClass_Invalid && deviceInfos[id]->deviceClass == vr::TrackedDeviceClass_Invalid)
				{
					if (deviceClass == vr::TrackedDeviceClass_HMD || deviceClass == vr::TrackedDeviceClass_Controller || deviceClass == vr::TrackedDeviceClass_GenericTracker)
					{
						auto info = std::make_shared<DeviceInfo>();
						info->openvrId = id;
						info->deviceClass = deviceClass;
						char buffer[vr::k_unMaxPropertyStringSize];

						// Get and save the serial number
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

						// Get and save the current device mode
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

						// Store the found info
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

				// Create new maps
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

	void DeviceManipulationTabController::reloadMotionCompensationSettings()
	{
		QSettings* settings = OverlayController::appSettings();
		settings->beginGroup("deviceManipulationSettings");
		
		// Load serials
		_HMDSerial = settings->value("motionCompensationHMDSerial", "").toString();
		_RefTrackerSerial = settings->value("motionCompensationRefTrackerSerial", "").toString();

		// Load filter settings
		_LPFBeta = settings->value("motionCompensationLPFBeta", 0.85).toDouble();
		_samples = settings->value("motionCompensationSamples", 12).toUInt();

		// Load offset settings
		_offset.Translation.v[0] = settings->value("motionCompensationOffsetTranslation_X", 0.0).toDouble();
		_offset.Translation.v[1] = settings->value("motionCompensationOffsetTranslation_Y", 0.0).toDouble();
		_offset.Translation.v[2] = settings->value("motionCompensationOffsetTranslation_Z", 0.0).toDouble();
		_offset.Rotation.v[0] = settings->value("motionCompensationOffsetRotation_P", 0.0).toDouble();
		_offset.Rotation.v[1] = settings->value("motionCompensationOffsetRotation_Y", 0.0).toDouble();
		_offset.Rotation.v[2] = settings->value("motionCompensationOffsetRotation_R", 0.0).toDouble();

		settings->endGroup();

		// Load shortcuts
		settings->beginGroup("motionCompensationShortcuts");

		Qt::Key shortcutKey = (Qt::Key)settings->value("shortcut_0_key", Qt::Key::Key_unknown).toInt();
		Qt::KeyboardModifiers shortcutMod = settings->value("shortcut_0_mod", Qt::KeyboardModifier::NoModifier).toInt();
		newKey(0, shortcutKey, shortcutMod);

		shortcutKey = (Qt::Key)settings->value("shortcut_1_key", Qt::Key::Key_unknown).toInt();
		Qt::KeyboardModifiers shortcutMod_2 = settings->value("shortcut_1_mod", Qt::KeyboardModifier::NoModifier).toInt();
		newKey(1, shortcutKey, shortcutMod_2);

		settings->endGroup();
		LOG(INFO) << "Loading saved Settings";
	}

	void DeviceManipulationTabController::saveMotionCompensationSettings()
	{
		LOG(INFO) << "Saving Settings";

		QSettings* settings = OverlayController::appSettings();
		settings->beginGroup("deviceManipulationSettings");
		
		// Save serials
		settings->setValue("motionCompensationHMDSerial", _HMDSerial);
		settings->setValue("motionCompensationRefTrackerSerial", _RefTrackerSerial);

		// Save filter settings
		settings->setValue("motionCompensationLPFBeta", _LPFBeta);
		settings->setValue("motionCompensationSamples", _samples);

		// Save offset settings
		settings->setValue("motionCompensationOffsetTranslation_X", _offset.Translation.v[0]);
		settings->setValue("motionCompensationOffsetTranslation_Y", _offset.Translation.v[1]);
		settings->setValue("motionCompensationOffsetTranslation_Z", _offset.Translation.v[2]);
		settings->setValue("motionCompensationOffsetRotation_P", _offset.Rotation.v[0]);
		settings->setValue("motionCompensationOffsetRotation_Y", _offset.Rotation.v[1]);
		settings->setValue("motionCompensationOffsetRotation_R", _offset.Rotation.v[2]);

		settings->endGroup();
		settings->sync();

		saveMotionCompensationShortcuts();			
	}

	void DeviceManipulationTabController::saveMotionCompensationShortcuts()
	{
		QSettings* settings = OverlayController::appSettings();
		settings->beginGroup("motionCompensationShortcuts");

		// Save shortcuts
		settings->setValue("shortcut_0_key", getKey_AsKey(0));
		settings->setValue("shortcut_0_mod", (int)getModifiers_AsModifiers(0));
		settings->setValue("shortcut_1_key", getKey_AsKey(1));
		settings->setValue("shortcut_1_mod", (int)getModifiers_AsModifiers(1));

		settings->endGroup();
		settings->sync();
	}

	void DeviceManipulationTabController::InitShortcuts()
	{
		NewShortcut(0, &DeviceManipulationTabController::toggleMotionCompensationMode, "Enable / Disable Motion Compensation");
		NewShortcut(1, &DeviceManipulationTabController::resetRefZeroPose, "Reset reference zero pose");
	}

	void DeviceManipulationTabController::NewShortcut(int id, void (DeviceManipulationTabController::* method)(), QString description)
	{
		shortcut[id].shortcut = new QGlobalShortcut(this);
		shortcut[id].description = description;
		shortcut[id].method = method;

		if (shortcut[id].isConnected)
		{
			disconnect(shortcut[id].connectionHandler);
		}

		ConnectShortcut(id);
	}

	void DeviceManipulationTabController::ConnectShortcut(int id)
	{
		shortcut[id].connectionHandler = connect(shortcut[id].shortcut, &QGlobalShortcut::activated, this, shortcut[id].method);
		shortcut[id].isConnected = true;
	}

	void DeviceManipulationTabController::DisconnectShortcut(int id)
	{
		disconnect(shortcut[id].connectionHandler);
		shortcut[id].isConnected = false;
	}

	void DeviceManipulationTabController::newKey(int id, Qt::Key key, Qt::KeyboardModifiers modifier)
	{
		if (key != Qt::Key::Key_unknown)
		{
			shortcut[id].key = key;
			shortcut[id].modifiers = modifier;

			shortcut[id].shortcut->setShortcut(QKeySequence(key + modifier));

			saveMotionCompensationShortcuts();
		}
	}

	void DeviceManipulationTabController::removeKey(int id)
	{
		shortcut[id].shortcut->unsetShortcut();
		shortcut[id].key = Qt::Key::Key_unknown;
		shortcut[id].modifiers = Qt::KeyboardModifier::NoModifier;
	}

	QString DeviceManipulationTabController::getStringFromKey(Qt::Key key)
	{
		return QKeySequence(key).toString();
	}

	QString DeviceManipulationTabController::getStringFromModifiers(Qt::KeyboardModifiers key)
	{
		return QKeySequence(key).toString();
	}

	Qt::Key DeviceManipulationTabController::getKey_AsKey(int id)
	{
		return shortcut[id].key;
	}

	QString DeviceManipulationTabController::getKey_AsString(int id)
	{
		if (shortcut[id].shortcut->isEmpty())
		{
			return "Empty";
		}

		return QKeySequence(shortcut[id].key).toString();
	}

	Qt::KeyboardModifiers DeviceManipulationTabController::getModifiers_AsModifiers(int id)
	{
		return shortcut[id].modifiers;
	}

	QString DeviceManipulationTabController::getModifiers_AsString(int id)
	{
		return QKeySequence(shortcut[id].modifiers).toString();
	}

	QString DeviceManipulationTabController::getKeyDescription(int id)
	{
		return shortcut[id].description;
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

	void DeviceManipulationTabController::setTrackerArrayID(unsigned OpenVRId, unsigned ArrayID)
	{
		TrackerArrayIdToDeviceId.insert(std::make_pair(ArrayID, OpenVRId));
		LOG(DEBUG) << "Set Tracker Array ID, OpenVR ID: " << OpenVRId << ", Array ID: " << ArrayID;
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

	void DeviceManipulationTabController::setReferenceTracker(unsigned openVRId)
	{
		_RefTrackerSerial = QString::fromStdString(deviceInfos[openVRId]->serial);
	}

	void DeviceManipulationTabController::setHMDArrayID(unsigned OpenVRId, unsigned ArrayID)
	{
		HMDArrayIdToDeviceId.insert(std::make_pair(ArrayID, OpenVRId));
		LOG(DEBUG) << "Set HMD Array ID, OpenVR ID: " << OpenVRId << ", Array ID: " << ArrayID;
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
	
	void DeviceManipulationTabController::setHMD(unsigned openVRId)
	{
		_HMDSerial = QString::fromStdString(deviceInfos[openVRId]->serial);
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

	void DeviceManipulationTabController::toggleMotionCompensationMode()
	{
		int MCid = -1;
		int RTid = -1;

		LOG(DEBUG) << "ToggleMC: HMD Serial: " << _HMDSerial.toStdString();
		LOG(DEBUG) << "ToggleMC: Ref Tracker Serial: " << _RefTrackerSerial.toStdString();

		// If the dashboard is not open, we need to refresh the device list
		// New connected devices are otherwise not displayed
		if (!parent->isDashboardVisible() || !parent->isDesktopMode())
		{
			SearchDevices();
		}

		// Search for the correct serial number and save its OpenVR Id.
		if (_RefTrackerSerial != "" && _HMDSerial != "")
		{
			for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; i++)
			{
				if (deviceInfos[i]->serial.compare(_HMDSerial.toStdString()) == 0)
				{
					MCid = i;
				}
				if (deviceInfos[i]->serial.compare(_RefTrackerSerial.toStdString()) == 0)
				{
					RTid = i;
				}
			}
		}

		if (MCid >= 0 && RTid >= 0 && MCid != RTid)
		{
			LOG(DEBUG) << "ToggleMC: Found both devices. HMD OVRID: " << MCid << ". Ref Tracker OVRID: " << RTid;

			applySettings_ovrid(MCid, RTid, !_MotionCompensationIsOn);
		}

		//int MCindex = QQmlProperty::read(parent, "hmdSelectionComboBox.currentIndex").toInt();
	}

	// Enables or disables the motion compensation for the selected device
	bool DeviceManipulationTabController::applySettings(unsigned MCindex, unsigned RTindex, bool EnableMotionCompensation)
	{
		unsigned RTid = 0;
		unsigned MCid = 0;

		// A few checks if the user input is valid
		if (MCindex < 0)
		{
			m_deviceModeErrorString = "Please select a device";
			return false;
		}

		auto search = HMDArrayIdToDeviceId.find(MCindex);
		if (search != HMDArrayIdToDeviceId.end())
		{
			MCid = search->second;
		}
		else
		{
			m_deviceModeErrorString = "Invalid internal reference for MC";
			return false;
		}

		LOG(DEBUG) << "Got this OpenVR ID for HMD: " << MCid;

		// Input validation for tracker
		if (_motionCompensationMode == vrmotioncompensation::MotionCompensationMode::ReferenceTracker)
		{
			if (RTindex < 0)
			{
				m_deviceModeErrorString = "Please select a reference tracker";
				return false;
			}

			// Search for the device ID
			search = TrackerArrayIdToDeviceId.find(RTindex);
			if (search != TrackerArrayIdToDeviceId.end())
			{
				RTid = search->second;
			}
			else
			{
				m_deviceModeErrorString = "Invalid internal reference for RT";
				return false;
			}

			LOG(DEBUG) << "Got this OpenVR ID for Tracker: " << RTid;

			if (MCid == RTid)
			{
				m_deviceModeErrorString = "\"Device\" and \"Reference Tracker\" cannot be the same!";
				return false;
			}

			if (deviceInfos[RTid]->deviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_HMD)
			{
				m_deviceModeErrorString = "\"Reference Tracker\" cannot be a HMD!";
				return false;
			}

			if (deviceInfos[RTid]->deviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_Invalid)
			{
				m_deviceModeErrorString = "\"Reference Tracker\" is invalid!";
				return false;
			}
		}

		if (deviceInfos[MCid]->deviceClass != vr::ETrackedDeviceClass::TrackedDeviceClass_HMD)
		{
			m_deviceModeErrorString = "\"Device\" is not a HMD!";
			return false;
		}

		return applySettings_ovrid(MCid, RTid, EnableMotionCompensation);
	}

	bool DeviceManipulationTabController::applySettings_ovrid(unsigned MCid, unsigned RTid, bool EnableMotionCompensation)
	{
		try
		{
			vrmotioncompensation::MotionCompensationMode NewMode = vrmotioncompensation::MotionCompensationMode::ReferenceTracker;

			// Send new settings to the driver.dll
			if (EnableMotionCompensation && _motionCompensationMode == vrmotioncompensation::MotionCompensationMode::ReferenceTracker)
			{
				LOG(INFO) << "Sending Motion Compensation Mode: ReferenceTracker";
			}
			else
			{
				LOG(INFO) << "Sending Motion Compensation Mode: Disabled";

				NewMode = vrmotioncompensation::MotionCompensationMode::Disabled;
			}

			// Send new mode
			parent->vrMotionCompensation().setDeviceMotionCompensationMode(deviceInfos[MCid]->openvrId, deviceInfos[RTid]->openvrId, NewMode);

			// Send settings
			parent->vrMotionCompensation().setMoticonCompensationSettings(_LPFBeta, _samples, _setZeroMode);
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
		catch (std::exception& e)
		{
			m_deviceModeErrorString = "Unknown exception";
			LOG(ERROR) << "Unknown exception caught while setting device mode: " << e.what();

			return false;
		}

		_MotionCompensationIsOn = EnableMotionCompensation;

		setHMD(MCid);
		setReferenceTracker(RTid);

		saveMotionCompensationSettings();

		return true;
	}

	void DeviceManipulationTabController::resetRefZeroPose()
	{
		try
		{
			LOG(INFO) << "Resetting reference zero pose";
			parent->vrMotionCompensation().resetRefZeroPose();
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
			}
		}
		catch (std::exception& e)
		{
			m_deviceModeErrorString = "Unknown exception";
			LOG(ERROR) << "Exception caught while resetting reference zero pose: " << e.what();
		}
	}

	QString DeviceManipulationTabController::getDeviceModeErrorString()
	{
		return m_deviceModeErrorString;
	}
	
	bool DeviceManipulationTabController::isDesktopModeActive()
	{
		return parent->isDesktopMode();
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

		_LPFBeta = value;

		return true;
	}

	double DeviceManipulationTabController::getLPFBeta()
	{
		return _LPFBeta;
	}

	bool DeviceManipulationTabController::setSamples(unsigned value)
	{
		// A few checks if the user input is valid
		if (value < 1)
		{
			m_deviceModeErrorString = "Samples cannot be lower than 1";
			return false;
		}

		_samples = value;

		return true;
	}

	unsigned DeviceManipulationTabController::getSamples()
	{
		return _samples;
	}

	void DeviceManipulationTabController::setZeroMode(bool setZero)
	{
		_setZeroMode = setZero;
	}

	bool DeviceManipulationTabController::getZeroMode()
	{
		return _setZeroMode;
	}

	void DeviceManipulationTabController::increaseLPFBeta(double value)
	{
		_LPFBeta += value;

		if (_LPFBeta > 1.0)
		{
			_LPFBeta = 1.0;
		}
		else if (_LPFBeta < 0.0)
		{
			_LPFBeta = 0.0;
		}

		emit settingChanged();
	}

	void DeviceManipulationTabController::increaseSamples(int value)
	{
		_samples += value;

		if (_samples <= 2)
		{
			_samples = 2;
		}

		emit settingChanged();
	}

	void DeviceManipulationTabController::setHMDtoRefTranslationOffset(unsigned axis, double value)
	{
		_offset.Translation.v[axis] = value;
		parent->vrMotionCompensation().setOffsets(_offset);
	}

	void DeviceManipulationTabController::setHMDtoRefRotationOffset(unsigned axis, double value)
	{
		_offset.Rotation.v[axis] = value;
		parent->vrMotionCompensation().setOffsets(_offset);
	}

	void DeviceManipulationTabController::increaseRefTranslationOffset(unsigned axis, double value)
	{
		_offset.Translation.v[axis] += value;
		parent->vrMotionCompensation().setOffsets(_offset);

		emit offsetChanged();
	}

	void DeviceManipulationTabController::increaseRefRotationOffset(unsigned axis, double value)
	{
		_offset.Rotation.v[axis] += value;
		parent->vrMotionCompensation().setOffsets(_offset);

		emit offsetChanged();
	}

	double DeviceManipulationTabController::getHMDtoRefTranslationOffset(unsigned axis)
	{
		return _offset.Translation.v[axis];
	}

	double DeviceManipulationTabController::getHMDtoRefRotationOffset(unsigned axis)
	{
		return _offset.Rotation.v[axis];
	}

	void DeviceManipulationTabController::setMotionCompensationMode(unsigned NewMode)
	{
		switch (NewMode)
		{
		case 0:
			_motionCompensationMode = vrmotioncompensation::MotionCompensationMode::ReferenceTracker;
			break;
		default:
			break;
		}
	}

	int DeviceManipulationTabController::getMotionCompensationMode()
	{
		switch (_motionCompensationMode)
		{
		case vrmotioncompensation::MotionCompensationMode::Disabled:
			return 0;
			break;
		case vrmotioncompensation::MotionCompensationMode::ReferenceTracker:
			return 0;
			break;
		default:
			return 0;
			break;
		}
	}

	bool DeviceManipulationTabController::setDebugMode(bool TestForStandby)
	{
		bool enable = false;
		QString newButtonText = "";
		int newLoggerStatus = 0;

		// Queue new debug logger state
		if (TestForStandby && _motionCompensationMode == vrmotioncompensation::MotionCompensationMode::ReferenceTracker && DebugLoggerStatus == 1)
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
		else if (!TestForStandby && _motionCompensationMode == vrmotioncompensation::MotionCompensationMode::Disabled && DebugLoggerStatus == 0)
		{
			newLoggerStatus = 1;
			newButtonText = "Standby...";
		}
		else if ((!TestForStandby && _motionCompensationMode == vrmotioncompensation::MotionCompensationMode::Disabled && DebugLoggerStatus == 1) ||
				 (!TestForStandby && _motionCompensationMode == vrmotioncompensation::MotionCompensationMode::ReferenceTracker && DebugLoggerStatus == 2))
		{
			enable = false;
			newLoggerStatus = 0;
			newButtonText = "Start logging";
		}
		else if (!TestForStandby && _motionCompensationMode == vrmotioncompensation::MotionCompensationMode::ReferenceTracker && DebugLoggerStatus == 0)
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
} // namespace motioncompensation