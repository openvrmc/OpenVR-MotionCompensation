#pragma once

#include <map>
#include <mutex>
#include <openvr_driver.h>
#include <vrinputemulator_types.h>
#include "utils/DevicePropertyValueVisitor.h"



// driver namespace
namespace vrinputemulator {
namespace driver {


// forward declarations
class ServerDriver;


/**
* Implements the ITrackedDeviceServerDriver interface.
*
* Represents a single virtual device managed by this driver. It has a serial number, a device type, device properties, a pose and components.
**/
class VirtualDeviceDriver : public vr::ITrackedDeviceServerDriver, public vr::IVRDriverInput {
protected:
	std::recursive_mutex _mutex;

	ServerDriver* m_serverDriver;
	VirtualDeviceType m_deviceType;
	std::string m_serialNumber;
	uint32_t m_virtualDeviceId;
	uint32_t m_openvrId = vr::k_unTrackedDeviceIndexInvalid;
	bool m_published = false;
	bool m_periodicPoseUpdates = true;
	vr::PropertyContainerHandle_t m_propertyContainer = vr::k_ulInvalidPropertyContainer;

	vr::DriverPose_t m_pose;
	typedef boost::variant<int32_t, uint64_t, float, bool, std::string, vr::HmdMatrix34_t, vr::HmdMatrix44_t, vr::HmdVector3_t, vr::HmdVector4_t> _devicePropertyType_t;
	std::map<int, _devicePropertyType_t> _deviceProperties;

	vr::VRControllerState_t m_ControllerState;

public:
	VirtualDeviceDriver(ServerDriver* parent, VirtualDeviceType type, const std::string& serial, uint32_t virtualId = vr::k_unTrackedDeviceIndexInvalid);


	// Inherited via ITrackedDeviceServerDriver
	virtual vr::EVRInitError Activate(uint32_t unObjectId) override;
	virtual void Deactivate() override;
	virtual void EnterStandby() override;
	virtual void* GetComponent(const char* pchComponentNameAndVersion) override;
	virtual void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) override;
	virtual vr::DriverPose_t GetPose() override;
	//Inherited via IVRDriverInputvirtual
	virtual vr::EVRInputError CreateBooleanComponent(vr::PropertyContainerHandle_t ulContainer, const char* pchName, vr::VRInputComponentHandle_t* pHandle) override;
	virtual vr::EVRInputError UpdateBooleanComponent(vr::VRInputComponentHandle_t ulComponent, bool bNewValue, double fTimeOffset) override;
	virtual vr::EVRInputError CreateScalarComponent(vr::PropertyContainerHandle_t ulContainer, const char* pchName, vr::VRInputComponentHandle_t* pHandle, vr::EVRScalarType eType, vr::EVRScalarUnits eUnits) override;
	virtual vr::EVRInputError UpdateScalarComponent(vr::VRInputComponentHandle_t ulComponent, float fNewValue, double fTimeOffset) override;
	virtual vr::EVRInputError CreateHapticComponent(vr::PropertyContainerHandle_t ulContainer, const char* pchName, vr::VRInputComponentHandle_t* pHandle) override;
	virtual vr::EVRInputError CreateSkeletonComponent(vr::PropertyContainerHandle_t ulContainer, const char* pchName, const char* pchSkeletonPath, const char* pchBasePosePath, vr::EVRSkeletalTrackingLevel eSkeletalTrackingLevel, const vr::VRBoneTransform_t* pGripLimitTransforms, uint32_t unGripLimitTransformCount, vr::VRInputComponentHandle_t* pHandle) override;
	virtual vr::EVRInputError UpdateSkeletonComponent(vr::VRInputComponentHandle_t ulComponent, vr::EVRSkeletalMotionRange eMotionRange, const vr::VRBoneTransform_t* pTransforms, uint32_t unTransformCount) override;

	// from self

	const std::string& serialNumber() { return m_serialNumber; }
	VirtualDeviceType deviceType() { return m_deviceType; }
	bool published() { return m_published; }
	bool periodicPoseUpdates() { return m_periodicPoseUpdates; }

	ServerDriver* serverDriver() { return m_serverDriver; }
	uint32_t openvrDeviceId() { return m_openvrId; }
	uint32_t virtualDeviceId() { return m_virtualDeviceId; }

	vr::DriverPose_t& driverPose() { return m_pose; }

	bool enablePeriodicPoseUpdates(bool enabled) { return m_periodicPoseUpdates; }
	void publish();

	void updatePose(const vr::DriverPose_t& newPose, double timeOffset, bool notify = true);
	void sendPoseUpdate(double timeOffset = 0.0, bool onlyWhenConnected = true);

	template<class T>
	T getTrackedDeviceProperty(vr::ETrackedDeviceProperty prop, vr::ETrackedPropertyError * pError) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		if (pError) {
			*pError = vr::TrackedProp_Success;
		}
		auto p = _deviceProperties.find((int)prop);
		if (p != _deviceProperties.end()) {
			try {
				T retval = boost::get<T>(p->second);
				return retval;
			} catch (std::exception&) {
				if (pError) {
					*pError = vr::TrackedProp_WrongDataType;
				}
				return T();
			}
		} else {
			if (pError) {
				*pError = vr::TrackedProp_ValueNotProvidedByDevice;
			}
			return T();
		}
	}

	template<class T>
	void setTrackedDeviceProperty(vr::ETrackedDeviceProperty prop, const T& value, bool notify = true) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		_deviceProperties[prop] = value;
		if (notify && m_openvrId != vr::k_unTrackedDeviceIndexInvalid) {
			auto errorMessage = boost::apply_visitor(DevicePropertyValueVisitor(m_propertyContainer, prop), _deviceProperties[prop]);
			if (!errorMessage.empty()) {
				LOG(ERROR) << "Could not set tracked device property: " << errorMessage;
			}
		}
	}

	void removeTrackedDeviceProperty(vr::ETrackedDeviceProperty prop, bool notify = true) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		auto i = _deviceProperties.find(prop);
		if (i != _deviceProperties.end()) {
			_deviceProperties.erase(i);
			if (notify && m_openvrId != vr::k_unTrackedDeviceIndexInvalid) {
				vr::VRProperties()->EraseProperty(m_propertyContainer, prop);
			}
		}
	}

	vr::VRControllerState_t& controllerState() { return m_ControllerState; }

};


} // end namespace driver
} // end namespace vrinputemulator
