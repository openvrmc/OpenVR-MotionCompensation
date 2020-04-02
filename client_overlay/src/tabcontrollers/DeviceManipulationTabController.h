#pragma once

#include <QObject>
#include <memory>
#include <openvr.h>
#include <vrmotioncompensation.h>
#include <vrmotioncompensation_types.h>

class QQuickWindow;
// application namespace
namespace motioncompensation
{
	// forward declaration
	class OverlayController;

	struct DeviceManipulationProfile
	{
		std::string profileName;
		int deviceMode = 0;
		int motionCompensationMode = 0;
	};

	struct DeviceInfo
	{
		std::string serial;
		vr::ETrackedDeviceClass deviceClass = vr::TrackedDeviceClass_Invalid;
		uint32_t openvrId = 0;
		int deviceStatus = 0;					// 0: Normal, 1: Disconnected/Suspended
		vrmotioncompensation::MotionCompensationDeviceMode deviceMode = vrmotioncompensation::MotionCompensationDeviceMode::Default;
		//uint32_t refDeviceId = 0;
		//uint32_t renderModelIndex = 0;
		//vr::VROverlayHandle_t renderModelOverlay = vr::k_ulOverlayHandleInvalid;
		//std::string renderModelOverlayName;
	};

	class DeviceManipulationTabController : public QObject
	{
		Q_OBJECT

	private:
		OverlayController* parent;
		QQuickWindow* widget;

		std::vector<std::shared_ptr<DeviceInfo>> deviceInfos;
		uint32_t maxValidDeviceId = 0;
		std::map<uint32_t, uint32_t> TrackerArrayIdToDeviceId;
		std::map<uint32_t, uint32_t> HMDArrayIdToDeviceId;

		std::vector<DeviceManipulationProfile> deviceManipulationProfiles;

		vrmotioncompensation::MotionCompensationMode motionCompensationMode = vrmotioncompensation::MotionCompensationMode::Disabled;
		double LPFBeta = 0.2;

		QString m_deviceModeErrorString;

		unsigned settingsUpdateCounter = 0;

		std::thread identifyThread;

	public:
		~DeviceManipulationTabController();

		void initStage1();

		void initStage2(OverlayController* parent, QQuickWindow* widget);

		void eventLoopTick(vr::TrackedDevicePose_t* devicePoses);

		bool SearchDevices(int StartID);

		void handleEvent(const vr::VREvent_t& vrEvent);

		Q_INVOKABLE unsigned getDeviceCount();
		Q_INVOKABLE QString getDeviceSerial(unsigned index);
		Q_INVOKABLE unsigned getOpenVRId(unsigned index);
		Q_INVOKABLE int getDeviceClass(unsigned index);
		Q_INVOKABLE int getDeviceState(unsigned index);
		Q_INVOKABLE int getDeviceMode(unsigned index);
		Q_INVOKABLE double getLPFBeta();
		Q_INVOKABLE void setTrackerArrayID(unsigned deviceID, unsigned ArrayID);
		Q_INVOKABLE void setHMDArrayID(unsigned deviceID, unsigned ArrayID);
		Q_INVOKABLE int getTrackerDeviceID(unsigned ArrayID);
		Q_INVOKABLE int getHMDDeviceID(unsigned ArrayID);
		Q_INVOKABLE void increaseLPFBeta(double value);

		void reloadMotionCompensationSettings();
		void reloadDeviceManipulationProfiles();
		void saveMotionCompensationSettings();
		void saveDeviceManipulationProfiles();

		Q_INVOKABLE unsigned getDeviceManipulationProfileCount();
		Q_INVOKABLE QString getDeviceManipulationProfileName(unsigned index);

		Q_INVOKABLE bool updateDeviceInfo(unsigned index);

		Q_INVOKABLE bool setMotionCompensationMode(unsigned Dindex, unsigned RTindex, bool EnableMotionCompensation/*, bool notify = true*/);
		Q_INVOKABLE bool setLPFBeta(double value);
		Q_INVOKABLE bool sendLPFBeta();
		Q_INVOKABLE QString getDeviceModeErrorString();

	public slots:
		void addDeviceManipulationProfile(QString name, unsigned deviceIndex, bool includesDeviceOffsets, bool includesInputRemapping);
		void applyDeviceManipulationProfile(unsigned index, unsigned deviceIndex);
		void deleteDeviceManipulationProfile(unsigned index);

	signals:
		//void loadComplete();
		void deviceCountChanged(unsigned deviceCount);
		void deviceInfoChanged(unsigned index);
		void deviceManipulationProfilesChanged();
	};
} // namespace motioncompensation