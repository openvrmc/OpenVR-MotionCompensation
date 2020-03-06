
#pragma once

#include <QObject>
#include <memory>
#include <openvr.h>
#include <vrinputemulator.h>

class QQuickWindow;
// application namespace
namespace inputemulator {

// forward declaration
class OverlayController;

struct DeviceManipulationProfile {
	std::string profileName;
	int deviceMode = 0;
	int motionCompensationMode = 0;
};


struct DeviceInfo {
	std::string serial;
	vr::ETrackedDeviceClass deviceClass = vr::TrackedDeviceClass_Invalid;
	uint32_t openvrId = 0;
	int deviceStatus = 0; // 0 .. Normal, 1 .. Disconnected/Suspended
	int deviceMode = 0; // 0 .. Default, 1 .. Motion Compensation
	uint32_t refDeviceId = 0;
	uint32_t renderModelIndex = 0;
	vr::VROverlayHandle_t renderModelOverlay = vr::k_ulOverlayHandleInvalid;
	std::string renderModelOverlayName;
};


class DeviceManipulationTabController : public QObject {
	Q_OBJECT

private:
	OverlayController* parent;
	QQuickWindow* widget;

	std::vector<std::shared_ptr<DeviceInfo>> deviceInfos;
	uint32_t maxValidDeviceId = 0;

	std::vector<DeviceManipulationProfile> deviceManipulationProfiles;

	vrinputemulator::MotionCompensationVelAccMode motionCompensationVelAccMode = vrinputemulator::MotionCompensationVelAccMode::Disabled;
	double motionCompensationKalmanProcessNoise = 0.1;
	double motionCompensationKalmanObservationNoise = 0.1;
	unsigned motionCompensationMovingAverageWindow = 3;

	QString m_deviceModeErrorString;

	unsigned settingsUpdateCounter = 0;

	std::thread identifyThread;

public:
	~DeviceManipulationTabController();
	void initStage1();
	void initStage2(OverlayController* parent, QQuickWindow* widget);

	void eventLoopTick(vr::TrackedDevicePose_t* devicePoses);
	void handleEvent(const vr::VREvent_t& vrEvent);

	Q_INVOKABLE unsigned getDeviceCount();
	Q_INVOKABLE QString getDeviceSerial(unsigned index);
	Q_INVOKABLE unsigned getDeviceId(unsigned index);
	Q_INVOKABLE int getDeviceClass(unsigned index);
	Q_INVOKABLE int getDeviceState(unsigned index);
	Q_INVOKABLE int getDeviceMode(unsigned index);
	Q_INVOKABLE unsigned getMotionCompensationVelAccMode();
	Q_INVOKABLE double getMotionCompensationKalmanProcessNoise();
	Q_INVOKABLE double getMotionCompensationKalmanObservationNoise();
	Q_INVOKABLE unsigned getMotionCompensationMovingAverageWindow();

	void reloadDeviceManipulationSettings();
	void reloadDeviceManipulationProfiles();
	void saveDeviceManipulationSettings();
	void saveDeviceManipulationProfiles();

	Q_INVOKABLE unsigned getDeviceManipulationProfileCount();
	Q_INVOKABLE QString getDeviceManipulationProfileName(unsigned index);

	Q_INVOKABLE unsigned getRenderModelCount();
	Q_INVOKABLE QString getRenderModelName(unsigned index);
	Q_INVOKABLE bool updateDeviceInfo(unsigned index);

	Q_INVOKABLE bool setDeviceMode(unsigned index, unsigned mode, unsigned targedIndex, bool notify = true);
	Q_INVOKABLE QString getDeviceModeErrorString();


public slots:
	void setDeviceRenderModel(unsigned deviceIndex, unsigned renderModelIndex);

	void addDeviceManipulationProfile(QString name, unsigned deviceIndex, bool includesDeviceOffsets, bool includesInputRemapping);
	void applyDeviceManipulationProfile(unsigned index, unsigned deviceIndex);
	void deleteDeviceManipulationProfile(unsigned index);

	void setMotionCompensationVelAccMode(unsigned mode, bool notify = true);
	void setMotionCompensationKalmanProcessNoise(double variance, bool notify = true);
	void setMotionCompensationKalmanObservationNoise(double variance, bool notify = true);
	void setMotionCompensationMovingAverageWindow(unsigned window, bool notify = true);

signals:
	void deviceCountChanged(unsigned deviceCount);
	void deviceInfoChanged(unsigned index);
	void motionCompensationSettingsChanged();
	void deviceManipulationProfilesChanged();
	void motionCompensationVelAccModeChanged(unsigned mode);
	void motionCompensationKalmanProcessNoiseChanged(double variance);
	void motionCompensationKalmanObservationNoiseChanged(double variance);
	void motionCompensationMovingAverageWindowChanged(unsigned window);

};

} // namespace inputemulator
