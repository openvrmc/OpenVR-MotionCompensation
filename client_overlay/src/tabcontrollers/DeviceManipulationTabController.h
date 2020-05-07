#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <openvr.h>
#include <vrmotioncompensation.h>
#include <vrmotioncompensation_types.h>
#include <vector>

class QQuickWindow;
// application namespace
namespace motioncompensation
{
	// forward declaration
	class OverlayController;

	struct DeviceInfo
	{
		std::string serial = "";
		vr::ETrackedDeviceClass deviceClass = vr::TrackedDeviceClass_Invalid;
		uint32_t openvrId = 0;
		int deviceStatus = 0;					// 0: Normal, 1: Disconnected/Suspended
		vrmotioncompensation::MotionCompensationDeviceMode deviceMode = vrmotioncompensation::MotionCompensationDeviceMode::Default;
	};

	class DeviceManipulationTabController : public QObject
	{
		Q_OBJECT

	private:
		OverlayController* parent;
		QQuickWindow* widget;

		std::vector<std::shared_ptr<DeviceInfo>> deviceInfos;
		std::map<uint32_t, uint32_t> TrackerArrayIdToDeviceId;
		std::map<uint32_t, uint32_t> HMDArrayIdToDeviceId;

		vrmotioncompensation::MotionCompensationMode motionCompensationMode = vrmotioncompensation::MotionCompensationMode::Disabled;
		double LPFBeta = 0.2;
		uint32_t samples = 100;
		bool setZeroMode = false;

		int DebugLoggerStatus = 0;		// 0 = Off; 1 = Standby; 2 = Running
		QString debugModeButtonString;

		QString m_deviceModeErrorString;

		unsigned settingsUpdateCounter = 0;

		std::thread identifyThread;

	public:
		~DeviceManipulationTabController();

		void initStage1();

		void initStage2(OverlayController* parent, QQuickWindow* widget);

		void eventLoopTick(vr::TrackedDevicePose_t* devicePoses);

		bool SearchDevices();

		void handleEvent(const vr::VREvent_t& vrEvent);

		Q_INVOKABLE unsigned getDeviceCount();
		Q_INVOKABLE QString getDeviceSerial(unsigned index);
		Q_INVOKABLE unsigned getOpenVRId(unsigned index);
		Q_INVOKABLE int getDeviceClass(unsigned index);
		Q_INVOKABLE int getDeviceState(unsigned index);
		Q_INVOKABLE int getDeviceMode(unsigned index);
		Q_INVOKABLE double getLPFBeta();
		Q_INVOKABLE unsigned getSamples();
		Q_INVOKABLE void setTrackerArrayID(unsigned deviceID, unsigned ArrayID);
		Q_INVOKABLE void setHMDArrayID(unsigned deviceID, unsigned ArrayID);
		Q_INVOKABLE int getTrackerDeviceID(unsigned ArrayID);
		Q_INVOKABLE int getHMDDeviceID(unsigned ArrayID);
		Q_INVOKABLE void increaseLPFBeta(double value);
		Q_INVOKABLE void increaseSamples(int value);

		void reloadMotionCompensationSettings();
		void saveMotionCompensationSettings();

		Q_INVOKABLE bool updateDeviceInfo(unsigned OpenVRId);

		Q_INVOKABLE bool setMotionCompensationMode(unsigned Dindex, unsigned RTindex, bool EnableMotionCompensation, bool setZero);
		Q_INVOKABLE bool setLPFBeta(double value);	
		Q_INVOKABLE bool setSamples(unsigned value);
		Q_INVOKABLE bool setDebugMode(bool TestForStandby);
		Q_INVOKABLE QString getDebugModeButtonText();
		Q_INVOKABLE bool sendMCSettings();
		Q_INVOKABLE QString getDeviceModeErrorString();

	public slots:

	signals:
		void deviceCountChanged();
		void deviceInfoChanged(unsigned index);
		void settingChanged();
		void debugModeChanged();
	};
} // namespace motioncompensation