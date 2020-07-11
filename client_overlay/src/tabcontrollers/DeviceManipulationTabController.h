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

		struct ShortcutStruct
		{
			Qt::Key key;
			Qt::KeyboardModifiers modifiers;
			QGlobalShortcut* shortcut;
			QString description;
			void (DeviceManipulationTabController::* method)();
			QMetaObject::Connection connectionHandler;
			bool isConnected;

			ShortcutStruct()
			{
				key = Qt::Key::Key_unknown;
				modifiers = Qt::KeyboardModifier::NoModifier;
				shortcut = nullptr;
				description = "No description provided";
				method = nullptr;
				isConnected = false;
			}
		};

	private:
		OverlayController* parent;
		QQuickWindow* widget;

		// Shortcut related
		ShortcutStruct shortcut[2];

		// Device and ID storage
		std::vector<std::shared_ptr<DeviceInfo>> deviceInfos;		// Holds all device infos. The index represents the OpenVR ID. Therefore there are many empty fields in this array
		std::map<uint32_t, uint32_t> TrackerArrayIdToDeviceId;
		std::map<uint32_t, uint32_t> HMDArrayIdToDeviceId;

		// Settings
		vrmotioncompensation::MotionCompensationMode _motionCompensationMode = vrmotioncompensation::MotionCompensationMode::ReferenceTracker;
		QString _RefTrackerSerial = "";
		QString _HMDSerial = "";
		double _LPFBeta = 0.2;
		uint32_t _samples = 100;
		bool _setZeroMode = false;
		vrmotioncompensation::MMFstruct_v1 _offset;
		bool _MotionCompensationIsOn = false;


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

		// Shortcut related functions
		void InitShortcuts();
		void NewShortcut(int id, void (DeviceManipulationTabController::* method)(), QString description);
		void ConnectShortcut(int id);
		void DisconnectShortcut(int id);

		Q_INVOKABLE void newKey(int id, Qt::Key key, Qt::KeyboardModifiers modifier);
		Q_INVOKABLE void removeKey(int id);
		Q_INVOKABLE QString getStringFromKey(Qt::Key key);
		Q_INVOKABLE QString getStringFromModifiers(Qt::KeyboardModifiers key);
		Q_INVOKABLE Qt::Key getKey_AsKey(int id);
		Q_INVOKABLE QString getKey_AsString(int id);
		Q_INVOKABLE Qt::KeyboardModifiers getModifiers_AsModifiers(int id);
		Q_INVOKABLE QString getModifiers_AsString(int id);
		Q_INVOKABLE QString getKeyDescription(int id);

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
		/*Q_INVOKABLE*/ void setReferenceTracker(unsigned openVRId);

		Q_INVOKABLE void setHMDArrayID(unsigned deviceID, unsigned ArrayID);
		Q_INVOKABLE int getHMDDeviceID(unsigned ArrayID);
		/*Q_INVOKABLE*/ void setHMD(unsigned openVRId);

		// General functions
		Q_INVOKABLE bool updateDeviceInfo(unsigned OpenVRId);
		void toggleMotionCompensationMode();
		Q_INVOKABLE bool applySettings(unsigned Dindex, unsigned RTindex, bool EnableMotionCompensation);
		bool applySettings_ovrid(unsigned MCid, unsigned RTid, bool EnableMotionCompensation);
		void resetRefZeroPose();
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

		Q_INVOKABLE void setHMDtoRefTranslationOffset(unsigned axis, double value);
		Q_INVOKABLE void setHMDtoRefRotationOffset(unsigned axis, double value);

		Q_INVOKABLE void increaseRefTranslationOffset(unsigned axis, double value);
		Q_INVOKABLE void increaseRefRotationOffset(unsigned axis, double value);

		Q_INVOKABLE double getHMDtoRefTranslationOffset(unsigned axis);
		Q_INVOKABLE double getHMDtoRefRotationOffset(unsigned axis);

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
		void offsetChanged();
		void debugModeChanged();
	};
} // namespace motioncompensation