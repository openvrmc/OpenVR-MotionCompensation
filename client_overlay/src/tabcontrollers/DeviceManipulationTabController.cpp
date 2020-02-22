#include "DeviceManipulationTabController.h"
#include <QQuickWindow>
#include <QApplication>
#include "../overlaycontroller.h"
#include <openvr_math.h>
#include <vrinputemulator_types.h>
#include <ipc_protocol.h>
#include <chrono>

// application namespace
namespace inputemulator
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
		reloadDeviceManipulationSettings();
	}

	void DeviceManipulationTabController::initStage2(OverlayController* parent, QQuickWindow* widget)
	{
		this->parent = parent;
		this->widget = widget;
		try
		{
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

						/*try {
							vrinputemulator::DeviceInfo info2;
							parent->vrInputEmulator().getDeviceInfo(info->openvrId, info2);
							info->deviceMode = info2.deviceMode;
						} catch (std::exception& e) {
							LOG(ERROR) << "Exception caught while getting device info: " << e.what();
						}*/

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
		}
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
					bool hasDeviceInfoChanged = updateDeviceInfo(i);
					unsigned status = devicePoses[info->openvrId].bDeviceIsConnected ? 0 : 1;
					if (info->deviceMode == 0 && info->deviceStatus != status)
					{
						info->deviceStatus = status;
						hasDeviceInfoChanged = true;
					}
					if (hasDeviceInfoChanged)
					{
						emit deviceInfoChanged(i);
					}
					++i;
				}
				bool newDeviceAdded = false;
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

							/*try {
								vrinputemulator::DeviceInfo info2;
								parent->vrInputEmulator().getDeviceInfo(info->openvrId, info2);
								info->deviceMode = info2.deviceMode;
							} catch (std::exception& e) {
								LOG(ERROR) << "Exception caught while getting device info: " << e.what();
							}*/

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
		}
		else
		{
			settingsUpdateCounter++;
		}
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

	unsigned DeviceManipulationTabController::getDeviceId(unsigned index)
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

	unsigned DeviceManipulationTabController::getMotionCompensationVelAccMode()
	{
		return (unsigned)motionCompensationVelAccMode;
	}

	double DeviceManipulationTabController::getMotionCompensationKalmanProcessNoise()
	{
		return motionCompensationKalmanProcessNoise;
	}

	double DeviceManipulationTabController::getMotionCompensationKalmanObservationNoise()
	{
		return motionCompensationKalmanObservationNoise;
	}

	unsigned DeviceManipulationTabController::getMotionCompensationMovingAverageWindow()
	{
		return motionCompensationMovingAverageWindow;
	}


	#define DEVICEMANIPULATIONSETTINGS_GETTRANSLATIONVECTOR(name) { \
	double valueX = settings->value(#name ## "_x", 0.0).toDouble(); \
	double valueY = settings->value(#name ## "_y", 0.0).toDouble(); \
	double valueZ = settings->value(#name ## "_z", 0.0).toDouble(); \
	entry.name = { valueX, valueY, valueZ }; \
}

	#define DEVICEMANIPULATIONSETTINGS_GETROTATIONVECTOR(name) { \
	double valueY = settings->value(#name ## "_yaw", 0.0).toDouble(); \
	double valueP = settings->value(#name ## "_pitch", 0.0).toDouble(); \
	double valueR = settings->value(#name ## "_roll", 0.0).toDouble(); \
	entry.name = { valueY, valueP, valueR }; \
}

	void DeviceManipulationTabController::reloadDeviceManipulationSettings()
	{
		auto settings = OverlayController::appSettings();
		settings->beginGroup("deviceManipulationSettings");
		motionCompensationVelAccMode = (vrinputemulator::MotionCompensationVelAccMode)settings->value("motionCompensationVelAccMode", 0).toUInt();
		motionCompensationKalmanProcessNoise = settings->value("motionCompensationKalmanProcessNoise", 0.1).toDouble();
		motionCompensationKalmanObservationNoise = settings->value("motionCompensationKalmanObservationNoise", 0.1).toDouble();
		motionCompensationMovingAverageWindow = settings->value("motionCompensationMovingAverageWindow", 3).toUInt();
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

	void DeviceManipulationTabController::saveDeviceManipulationSettings()
	{
		auto settings = OverlayController::appSettings();
		settings->beginGroup("deviceManipulationSettings");
		settings->setValue("motionCompensationVelAccMode", (unsigned)motionCompensationVelAccMode);
		settings->setValue("motionCompensationKalmanProcessNoise", motionCompensationKalmanProcessNoise);
		settings->setValue("motionCompensationKalmanObservationNoise", motionCompensationKalmanObservationNoise);
		settings->setValue("motionCompensationMovingAverageWindow", motionCompensationMovingAverageWindow);
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

	void DeviceManipulationTabController::setMotionCompensationVelAccMode(unsigned mode, bool notify)
	{
		vrinputemulator::MotionCompensationVelAccMode newMode = (vrinputemulator::MotionCompensationVelAccMode)mode;
		if (motionCompensationVelAccMode != newMode)
		{
			motionCompensationVelAccMode = newMode;
			LOG(INFO) << "Sending motion compensation vel/acc mode to driver";
			parent->vrInputEmulator().setMotionVelAccCompensationMode(newMode);
			saveDeviceManipulationSettings();
			if (notify)
			{
				emit motionCompensationVelAccModeChanged(mode);
			}
		}
	}

	void DeviceManipulationTabController::setMotionCompensationKalmanProcessNoise(double variance, bool notify)
	{
		if (motionCompensationKalmanProcessNoise != variance)
		{
			motionCompensationKalmanProcessNoise = variance;
			LOG(INFO) << "Sending motion compensation kalman mode to driver";
			parent->vrInputEmulator().setMotionCompensationKalmanProcessNoise(motionCompensationKalmanProcessNoise);
			saveDeviceManipulationSettings();
			if (notify)
			{
				emit motionCompensationKalmanProcessNoiseChanged(motionCompensationKalmanProcessNoise);
			}
		}
	}

	void DeviceManipulationTabController::setMotionCompensationKalmanObservationNoise(double variance, bool notify)
	{
		if (motionCompensationKalmanObservationNoise != variance)
		{
			motionCompensationKalmanObservationNoise = variance;
			LOG(INFO) << "Sending motion compensation kalman noise to driver";
			parent->vrInputEmulator().setMotionCompensationKalmanObservationNoise(motionCompensationKalmanObservationNoise);
			saveDeviceManipulationSettings();
			if (notify)
			{
				emit motionCompensationKalmanObservationNoiseChanged(motionCompensationKalmanObservationNoise);
			}
		}
	}

	void DeviceManipulationTabController::setMotionCompensationMovingAverageWindow(unsigned window, bool notify)
	{
		if (motionCompensationMovingAverageWindow != window)
		{
			motionCompensationMovingAverageWindow = window;
			LOG(INFO) << "Sending motion compensation moving average mode to driver";
			parent->vrInputEmulator().setMotionCompensationMovingAverageWindow(motionCompensationMovingAverageWindow);
			saveDeviceManipulationSettings();
			if (notify)
			{
				emit motionCompensationMovingAverageWindowChanged(motionCompensationMovingAverageWindow);
			}
		}
	}

	unsigned DeviceManipulationTabController::getRenderModelCount()
	{
		return (unsigned)vr::VRRenderModels()->GetRenderModelCount();
	}

	QString DeviceManipulationTabController::getRenderModelName(unsigned index)
	{
		char buffer[vr::k_unMaxPropertyStringSize];
		vr::VRRenderModels()->GetRenderModelName(index, buffer, vr::k_unMaxPropertyStringSize);
		return buffer;
	}

	// 0 .. normal, 1 .. disable, 2 .. redirect mode, 3 .. swap mode, 4 ... motion compensation
	bool DeviceManipulationTabController::setDeviceMode(unsigned index, unsigned mode, unsigned targedIndex, bool notify)
	{
		bool retval = true;
		try
		{
			switch (mode)
			{
			case 0:
				parent->vrInputEmulator().setDeviceNormalMode(deviceInfos[index]->openvrId);
				break;
			case 1:
				if (motionCompensationVelAccMode == vrinputemulator::MotionCompensationVelAccMode::KalmanFilter)
				{
					parent->vrInputEmulator().setMotionCompensationKalmanProcessNoise(motionCompensationKalmanProcessNoise);
					parent->vrInputEmulator().setMotionCompensationKalmanObservationNoise(motionCompensationKalmanObservationNoise);
					LOG(INFO) << "Set kalman mc mode";
				}
				else if (motionCompensationVelAccMode == vrinputemulator::MotionCompensationVelAccMode::LinearApproximation)
				{
					parent->vrInputEmulator().setMotionCompensationMovingAverageWindow(motionCompensationMovingAverageWindow);
					LOG(INFO) << "Set moving avg mc mode";
				}
				parent->vrInputEmulator().setDeviceMotionCompensationMode(deviceInfos[index]->openvrId, motionCompensationVelAccMode);
				break;
			default:
				retval = false;
				m_deviceModeErrorString = "Unknown Device Mode";
				LOG(ERROR) << "Unkown device mode";
				break;
			}
		}
		catch (vrinputemulator::vrinputemulator_exception & e)
		{
			retval = false;
			switch (e.errorcode)
			{
			case (int)vrinputemulator::ipc::ReplyStatus::Ok:
			{
				m_deviceModeErrorString = "Not an error";
			} break;
			case (int)vrinputemulator::ipc::ReplyStatus::AlreadyInUse:
			{
				m_deviceModeErrorString = "Device already in use";
			} break;
			case (int)vrinputemulator::ipc::ReplyStatus::InvalidId:
			{
				m_deviceModeErrorString = "Invalid Id";
			} break;
			case (int)vrinputemulator::ipc::ReplyStatus::NotFound:
			{
				m_deviceModeErrorString = "Device not found";
			} break;
			case (int)vrinputemulator::ipc::ReplyStatus::NotTracking:
			{
				m_deviceModeErrorString = "Device not tracking";
			} break;
			default:
			{
				m_deviceModeErrorString = "Unknown error";
			} break;
			}
			LOG(ERROR) << "Exception caught while setting device mode: " << e.what();
		}
		catch (std::exception & e)
		{
			retval = false;
			m_deviceModeErrorString = "Unknown exception";
			LOG(ERROR) << "Exception caught while setting device mode: " << e.what();
		}
		if (notify)
		{
			updateDeviceInfo(index);
			emit deviceInfoChanged(index);
		}
		return retval;
	}

	QString DeviceManipulationTabController::getDeviceModeErrorString()
	{
		return m_deviceModeErrorString;
	}


	bool DeviceManipulationTabController::updateDeviceInfo(unsigned index)
	{
		/*bool retval = false;
		if (index < deviceInfos.size()) {
			try {
				vrinputemulator::DeviceInfo info;
				parent->vrInputEmulator().getDeviceInfo(deviceInfos[index]->openvrId, info);
				if (deviceInfos[index]->deviceMode != info.deviceMode) {
					deviceInfos[index]->deviceMode = info.deviceMode;
					retval = true;
				}
			} catch (std::exception& e) {
				LOG(ERROR) << "Exception caught while getting device info: " << e.what();
			}
		}
		return retval;*/
		return true;
	}

	void DeviceManipulationTabController::setDeviceRenderModel(unsigned deviceIndex, unsigned renderModelIndex)
	{
		if (deviceIndex < deviceInfos.size())
		{
			if (renderModelIndex == 0)
			{
				if (deviceInfos[deviceIndex]->renderModelOverlay != vr::k_ulOverlayHandleInvalid)
				{
					vr::VROverlay()->DestroyOverlay(deviceInfos[deviceIndex]->renderModelOverlay);
					deviceInfos[deviceIndex]->renderModelOverlay = vr::k_ulOverlayHandleInvalid;
				}
			}
			else
			{
				vr::VROverlayHandle_t overlayHandle = deviceInfos[deviceIndex]->renderModelOverlay;
				if (overlayHandle == vr::k_ulOverlayHandleInvalid)
				{
					std::string overlayName = std::string("RenderModelOverlay_") + std::string(deviceInfos[deviceIndex]->serial);
					auto oerror = vr::VROverlay()->CreateOverlay(overlayName.c_str(), overlayName.c_str(), &overlayHandle);
					if (oerror == vr::VROverlayError_None)
					{
						overlayHandle = deviceInfos[deviceIndex]->renderModelOverlay = overlayHandle;
					}
					else
					{
						LOG(ERROR) << "Could not create render model overlay: " << vr::VROverlay()->GetOverlayErrorNameFromEnum(oerror);
					}
				}
				if (overlayHandle != vr::k_ulOverlayHandleInvalid)
				{
					std::string texturePath = QApplication::applicationDirPath().toStdString() + "\\res\\transparent.png";
					if (QFile::exists(QString::fromStdString(texturePath)))
					{
						vr::VROverlay()->SetOverlayFromFile(overlayHandle, texturePath.c_str());
						char buffer[vr::k_unMaxPropertyStringSize];
						vr::VRRenderModels()->GetRenderModelName(renderModelIndex - 1, buffer, vr::k_unMaxPropertyStringSize);
						vr::VROverlay()->SetOverlayRenderModel(overlayHandle, buffer, nullptr);
						vr::HmdMatrix34_t trans = {
							1.0f, 0.0f, 0.0f, 0.0f,
							0.0f, 1.0f, 0.0f, 0.0f,
							0.0f, 0.0f, 1.0f, 0.0f
						};
						vr::VROverlay()->SetOverlayTransformTrackedDeviceRelative(overlayHandle, deviceInfos[deviceIndex]->openvrId, &trans);
						vr::VROverlay()->ShowOverlay(overlayHandle);
					}
					else
					{
						LOG(ERROR) << "Could not find texture \"" << texturePath << "\"";
					}
				}
			}
		}
	}

} // namespace inputemulator