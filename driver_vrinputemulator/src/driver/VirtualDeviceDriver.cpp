#include "VirtualDeviceDriver.h"

#include "../logging.h"


namespace vrinputemulator {
namespace driver {


VirtualDeviceDriver::VirtualDeviceDriver(ServerDriver* parent, VirtualDeviceType type, const std::string& serial, uint32_t virtualId)
		: m_serverDriver(parent), m_deviceType(type), m_serialNumber(serial), m_virtualDeviceId(virtualId) {
	memset(&m_pose, 0, sizeof(vr::DriverPose_t));
	m_pose.qDriverFromHeadRotation.w = 1;
	m_pose.qWorldFromDriverRotation.w = 1;
	m_pose.qRotation.w = 1;
	m_pose.result = vr::ETrackingResult::TrackingResult_Uninitialized;
	memset(&m_ControllerState, 0, sizeof(vr::VRControllerState_t));
}

vr::EVRInitError VirtualDeviceDriver::Activate(uint32_t unObjectId) {
	LOG(TRACE) << "VirtualDeviceDriver[" << m_serialNumber << "]::Activate( " << unObjectId << " )";
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	m_propertyContainer = vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);
	m_openvrId = unObjectId;
	//m_serverDriver->_trackedDeviceActivated(m_openvrId, this);
	for (auto& e : _deviceProperties) {
		auto errorMessage = boost::apply_visitor(DevicePropertyValueVisitor(m_propertyContainer, (vr::ETrackedDeviceProperty)e.first), e.second);
		if (!errorMessage.empty()) {
			LOG(ERROR) << "Could not set tracked device property: " << errorMessage;
		}
	}
	return vr::VRInitError_None;
}


void VirtualDeviceDriver::Deactivate() {
	LOG(TRACE) << "VirtualDeviceDriver[" << m_serialNumber << "]::Deactivate()";
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	//m_serverDriver->_trackedDeviceDeactivated(m_openvrId);
	m_openvrId = vr::k_unTrackedDeviceIndexInvalid;
}

void * VirtualDeviceDriver::GetComponent(const char * pchComponentNameAndVersion) {
	LOG(TRACE) << "VirtualDeviceDriver[" << m_serialNumber << "]::GetComponent( " << pchComponentNameAndVersion << " )";
	if (0 == strcmp(vr::IVRDriverInput_Version, pchComponentNameAndVersion))
	{
		return (vr::IVRDriverInput*)this;
	}
	return nullptr;
}

void VirtualDeviceDriver::EnterStandby()
{
}

void VirtualDeviceDriver::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize)
{
	if (unResponseBufferSize >= 1)
		pchResponseBuffer[0] = 0;
}

vr::DriverPose_t VirtualDeviceDriver::GetPose() {
	LOG(TRACE) << "VirtualDeviceDriver[" << m_serialNumber << "]::GetPose()";
	return m_pose;
}


void VirtualDeviceDriver::updatePose(const vr::DriverPose_t & newPose, double timeOffset, bool notify) {
	LOG(TRACE) << "VirtualDeviceDriver[" << m_serialNumber << "]::updatePose( " << timeOffset << " )";
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	m_pose = newPose;
	m_pose.poseTimeOffset += timeOffset;
	if (notify && m_openvrId != vr::k_unTrackedDeviceIndexInvalid) {
		vr::VRServerDriverHost()->TrackedDevicePoseUpdated(m_openvrId, m_pose, sizeof(vr::DriverPose_t));
	}
}

void VirtualDeviceDriver::sendPoseUpdate(double timeOffset, bool onlyWhenConnected) {
	LOG(TRACE) << "VirtualDeviceDriver[" << m_serialNumber << "]::sendPoseUpdate( " << timeOffset << " )";
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	if (!onlyWhenConnected || (m_pose.poseIsValid && m_pose.deviceIsConnected)) {
		m_pose.poseTimeOffset = timeOffset;
		if (m_openvrId != vr::k_unTrackedDeviceIndexInvalid) {
			vr::VRServerDriverHost()->TrackedDevicePoseUpdated(m_openvrId, m_pose, sizeof(vr::DriverPose_t));
		}
	}
}

void VirtualDeviceDriver::publish() {
	LOG(TRACE) << "VirtualDeviceDriver[" << m_serialNumber << "]::publish()";
	if (!m_published) {
		vr::ETrackedPropertyError pError;
		vr::ETrackedDeviceClass deviceClass = (vr::ETrackedDeviceClass)getTrackedDeviceProperty<int32_t>(vr::Prop_DeviceClass_Int32, &pError);
		if (pError == vr::TrackedProp_Success) {
			vr::VRServerDriverHost()->TrackedDeviceAdded(m_serialNumber.c_str(), deviceClass, this);
			m_published = true;
		} else {
			throw std::runtime_error(std::string("Could not get device class (") + std::to_string((int)pError) + std::string(")"));
		}
	}
}

vr::EVRInputError VirtualDeviceDriver::CreateBooleanComponent(vr::PropertyContainerHandle_t ulContainer, const char* pchName, vr::VRInputComponentHandle_t* pHandle) {
	return vr::VRDriverInput()->CreateBooleanComponent(ulContainer, pchName, pHandle);
}

vr::EVRInputError VirtualDeviceDriver::UpdateBooleanComponent(vr::VRInputComponentHandle_t ulComponent, bool bNewValue, double fTimeOffset) {
	return vr::VRDriverInput()->UpdateBooleanComponent(ulComponent, bNewValue, fTimeOffset);
}

vr::EVRInputError VirtualDeviceDriver::CreateScalarComponent(vr::PropertyContainerHandle_t ulContainer, const char* pchName, vr::VRInputComponentHandle_t* pHandle, vr::EVRScalarType eType, vr::EVRScalarUnits eUnits) {
	return vr::VRDriverInput()->CreateScalarComponent(ulContainer, pchName, pHandle, eType, eUnits);
}

vr::EVRInputError VirtualDeviceDriver::UpdateScalarComponent(vr::VRInputComponentHandle_t ulComponent, float fNewValue, double fTimeOffset) {
	return vr::VRDriverInput()->UpdateScalarComponent(ulComponent, fNewValue, fTimeOffset);
}

vr::EVRInputError VirtualDeviceDriver::CreateHapticComponent(vr::PropertyContainerHandle_t ulContainer, const char* pchName, vr::VRInputComponentHandle_t* pHandle) {
	return vr::EVRInputError::VRInputError_None;
}

vr::EVRInputError VirtualDeviceDriver::CreateSkeletonComponent(vr::PropertyContainerHandle_t ulContainer, const char* pchName, const char* pchSkeletonPath, const char* pchBasePosePath, vr::EVRSkeletalTrackingLevel eSkeletalTrackingLevel, const vr::VRBoneTransform_t* pGripLimitTransforms, uint32_t unGripLimitTransformCount, vr::VRInputComponentHandle_t* pHandle) {
	return vr::EVRInputError::VRInputError_None;
}
vr::EVRInputError VirtualDeviceDriver::UpdateSkeletonComponent(vr::VRInputComponentHandle_t ulComponent, vr::EVRSkeletalMotionRange eMotionRange, const vr::VRBoneTransform_t* pTransforms, uint32_t unTransformCount) {
	return vr::EVRInputError::VRInputError_None;
}


} // end namespace driver
} // end namespace vrinputemulator
