#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <openvr.h>
#include <vrmotioncompensation.h>
#include <vrmotioncompensation_types.h>
#include <vector>
#include "src/QGlobalShortcut/qglobalshortcut.h"

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

		// Device and ID storage
		std::vector<std::shared_ptr<DeviceInfo>> deviceInfos;
		std::map<uint32_t, uint32_t> TrackerArrayIdToDeviceId;
		std::map<uint32_t, uint32_t> HMDArrayIdToDeviceId;

		// Settings
		vrmotioncompensation::MotionCompensationMode _motionCompensationMode = vrmotioncompensation::MotionCompensationMode::ReferenceTracker;
		vrmotioncompensation::MotionCompensationMode _motionCompensationModeOldMode = vrmotioncompensation::MotionCompensationMode::ReferenceTracker;
		double _LPFBeta = 0.2;
		uint32_t _samples = 100;
		bool _setZeroMode = false;
		vr::HmdVector3d_t _offset = { 0, 0, 0 };

		QGlobalShortcut* shortcutFirst;

		// Debug
		int DebugLoggerStatus = 0;		// 0 = Off; 1 = Standby; 2 = Running
		QString debugModeButtonString;

		// Error return string
		QString m_deviceModeErrorString;

		// Threads
		std::thread identifyThread;
		unsigned settingsUpdateCounter = 0;

	public:
		~DeviceManipulationTabController();

		void initStage1();

		void Beenden();

		void initStage2(OverlayController* parent, QQuickWindow* widget);

		void eventLoopTick(vr::TrackedDevicePose_t* devicePoses);

		bool SearchDevices();

		void handleEvent(const vr::VREvent_t& vrEvent);

		void reloadMotionCompensationSettings();

		void saveMotionCompensationSettings();

		// General getter and setter
		Q_INVOKABLE unsigned getDeviceCount();
		Q_INVOKABLE QString getDeviceSerial(unsigned index);
		Q_INVOKABLE unsigned getOpenVRId(unsigned index);
		Q_INVOKABLE int getDeviceClass(unsigned index);
		Q_INVOKABLE int getDeviceState(unsigned index);
		Q_INVOKABLE int getDeviceMode(unsigned index);		

		// Getter and setter related to the HMD and Tracker drop downs
		Q_INVOKABLE void setTrackerArrayID(unsigned deviceID, unsigned ArrayID);
		Q_INVOKABLE int getTrackerDeviceID(unsigned ArrayID);

		Q_INVOKABLE void setHMDArrayID(unsigned deviceID, unsigned ArrayID);
		Q_INVOKABLE int getHMDDeviceID(unsigned ArrayID);

		// General functions
		Q_INVOKABLE bool updateDeviceInfo(unsigned OpenVRId);
		Q_INVOKABLE bool applySettings(unsigned Dindex, unsigned RTindex, bool EnableMotionCompensation);		
		Q_INVOKABLE bool sendMCSettings();
		Q_INVOKABLE QString getDeviceModeErrorString();

		// Settings
		Q_INVOKABLE bool setLPFBeta(double value);
		Q_INVOKABLE double getLPFBeta();

		Q_INVOKABLE bool setSamples(unsigned value);
		Q_INVOKABLE unsigned getSamples();

		Q_INVOKABLE void setZeroMode(bool setZero);
		Q_INVOKABLE bool getZeroMode();

		Q_INVOKABLE void increaseLPFBeta(double value);
		Q_INVOKABLE void increaseSamples(int value);

		Q_INVOKABLE void setHMDtoRefOffset(double x, double y, double z);
		Q_INVOKABLE double getHMDtoRefOffset(unsigned axis);

		Q_INVOKABLE void setMotionCompensationMode(unsigned NewMode);
		Q_INVOKABLE int getMotionCompensationMode();

		// Debug mode
		Q_INVOKABLE bool setDebugMode(bool TestForStandby);
		Q_INVOKABLE QString getDebugModeButtonText();
		

	public slots:

	signals:
		void deviceCountChanged();
		void deviceInfoChanged(unsigned index);
		void settingChanged();
		void debugModeChanged();
	};
} // namespace motioncompensation