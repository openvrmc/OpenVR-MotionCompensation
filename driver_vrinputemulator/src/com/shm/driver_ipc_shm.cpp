#include "driver_ipc_shm.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <openvr_driver.h>
#include <ipc_protocol.h>
#include <openvr_math.h>
#include "../../driver/ServerDriver.h"
#include "../../devicemanipulation/DeviceManipulationHandle.h"


namespace vrinputemulator {
namespace driver {


void IpcShmCommunicator::init(ServerDriver* driver) {
	_driver = driver;
	_ipcThreadStopFlag = false;
	_ipcThread = std::thread(_ipcThreadFunc, this, driver);
}

void IpcShmCommunicator::shutdown() {
	if (_ipcThreadRunning) {
		_ipcThreadStopFlag = true;
		_ipcThread.join();
	}
}

void IpcShmCommunicator::sendReplySetMotionCompensationMode(bool success) {
	if (_setMotionCompensationMessageId != 0) {
		ipc::Reply resp(ipc::ReplyType::GenericReply);
		resp.messageId = _setMotionCompensationMessageId;
		if (success) {
			resp.status = ipc::ReplyStatus::Ok;
		} else {
			resp.status = ipc::ReplyStatus::NotTracking;
		}
		sendReply(_setMotionCompensationClientId, resp);
	}
	_setMotionCompensationMessageId = 0;
}

void IpcShmCommunicator::_ipcThreadFunc(IpcShmCommunicator* _this, ServerDriver * driver) {
	_this->_ipcThreadRunning = true;
	LOG(DEBUG) << "CServerDriver::_ipcThreadFunc: thread started";
	try {
		// Create message queue
		boost::interprocess::message_queue::remove(_this->_ipcQueueName.c_str());
		boost::interprocess::message_queue messageQueue(
			boost::interprocess::create_only,
			_this->_ipcQueueName.c_str(),
			100,					//max message number
			sizeof(ipc::Request)    //max message size
			);

		while (!_this->_ipcThreadStopFlag) {
			try {
				ipc::Request message;
				uint64_t recv_size;
				unsigned priority;
				boost::posix_time::ptime timeout = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::milliseconds(50);
				if (messageQueue.timed_receive(&message, sizeof(ipc::Request), recv_size, priority, timeout)) {
					LOG(TRACE) << "CServerDriver::_ipcThreadFunc: IPC request received ( type " << (int)message.type << ")";
					if (recv_size == sizeof(ipc::Request)) {
						switch (message.type) {

						case ipc::RequestType::IPC_ClientConnect:
							{
								try {
									auto queue = std::make_shared<boost::interprocess::message_queue>(boost::interprocess::open_only, message.msg.ipc_ClientConnect.queueName);
									ipc::Reply reply(ipc::ReplyType::IPC_ClientConnect);
									reply.messageId = message.msg.ipc_ClientConnect.messageId;
									reply.msg.ipc_ClientConnect.ipcProcotolVersion = IPC_PROTOCOL_VERSION;
									uint32_t clientId = 0;
									if (message.msg.ipc_ClientConnect.ipcProcotolVersion == IPC_PROTOCOL_VERSION) {
										clientId = _this->_ipcClientIdNext++;
										_this->_ipcEndpoints.insert({ clientId, queue });
										reply.msg.ipc_ClientConnect.clientId = clientId;
										reply.status = ipc::ReplyStatus::Ok;
										LOG(INFO) << "New client connected: endpoint \"" << message.msg.ipc_ClientConnect.queueName << "\", cliendId " << clientId;
									} else {
										reply.msg.ipc_ClientConnect.clientId = 0;
										reply.status = ipc::ReplyStatus::InvalidVersion;
										LOG(INFO) << "Client (endpoint \"" << message.msg.ipc_ClientConnect.queueName << "\") reports incompatible ipc version "
											<< message.msg.ipc_ClientConnect.ipcProcotolVersion;
									}
									_this->sendReply(clientId, reply);
								} catch (std::exception& e) {
									LOG(ERROR) << "Error during client connect: " << e.what();
								}
							}
							break;

						case ipc::RequestType::IPC_ClientDisconnect:
							{
								ipc::Reply reply(ipc::ReplyType::GenericReply);
								reply.messageId = message.msg.ipc_ClientDisconnect.messageId;
								auto i = _this->_ipcEndpoints.find(message.msg.ipc_ClientDisconnect.clientId);
								if (i != _this->_ipcEndpoints.end()) {
									reply.status = ipc::ReplyStatus::Ok;
									LOG(INFO) << "Client disconnected: clientId " << message.msg.ipc_ClientDisconnect.clientId;
									if (reply.messageId != 0) {
										_this->sendReply(message.msg.ipc_ClientDisconnect.clientId, reply);
									}
									_this->_ipcEndpoints.erase(i);
								} else {
									LOG(ERROR) << "Error during client disconnect: unknown clientID " << message.msg.ipc_ClientDisconnect.clientId;
								}
							}
							break;

						case ipc::RequestType::IPC_Ping:
							{
								LOG(TRACE) << "Ping received: clientId " << message.msg.ipc_Ping.clientId << ", nonce " << message.msg.ipc_Ping.nonce;
								ipc::Reply reply(ipc::ReplyType::IPC_Ping);
								reply.messageId = message.msg.ipc_Ping.messageId;
								reply.status = ipc::ReplyStatus::Ok;
								reply.msg.ipc_Ping.nonce = message.msg.ipc_Ping.nonce;
								_this->sendReply(message.msg.ipc_ClientDisconnect.clientId, reply);
							}
							break;

						case ipc::RequestType::OpenVR_VendorSpecificEvent:
							{
								driver->openvr_vendorSpecificEvent(message.msg.ovr_VendorSpecificEvent.deviceId, message.msg.ovr_VendorSpecificEvent.eventType,
									message.msg.ovr_VendorSpecificEvent.eventData, message.msg.ovr_VendorSpecificEvent.timeOffset);
							}
							break;

						case ipc::RequestType::DeviceManipulation_DefaultMode:
							{
								ipc::Reply resp(ipc::ReplyType::GenericReply);
								resp.messageId = message.msg.ovr_GenericDeviceIdMessage.messageId;
								if (message.msg.ovr_GenericDeviceIdMessage.deviceId >= vr::k_unMaxTrackedDeviceCount) {
									resp.status = ipc::ReplyStatus::InvalidId;
								} else {
									DeviceManipulationHandle* info = driver->getDeviceManipulationHandleById(message.msg.ovr_GenericDeviceIdMessage.deviceId);
									if (!info) {
										resp.status = ipc::ReplyStatus::NotFound;
									} else {
										info->setDefaultMode();
										resp.status = ipc::ReplyStatus::Ok;
									}
									LOG(INFO) << "Setting driver into default mode";
								}
								if (resp.status != ipc::ReplyStatus::Ok) {
									LOG(ERROR) << "Error while updating device pose offset: Error code " << (int)resp.status;
								}
								if (resp.messageId != 0) {
									_this->sendReply(message.msg.ovr_GenericDeviceIdMessage.clientId, resp);
								}
							}
							break;
								
						case ipc::RequestType::DeviceManipulation_MotionCompensationMode:
							{
								ipc::Reply resp(ipc::ReplyType::GenericReply);
								resp.messageId = message.msg.dm_MotionCompensationMode.messageId;
								if (message.msg.dm_MotionCompensationMode.deviceId >= vr::k_unMaxTrackedDeviceCount) {
									resp.status = ipc::ReplyStatus::InvalidId;
								} else {
									DeviceManipulationHandle* info = driver->getDeviceManipulationHandleById(message.msg.dm_MotionCompensationMode.deviceId);
									if (!info) {
										resp.status = ipc::ReplyStatus::NotFound;
									} else {
										auto serverDriver = ServerDriver::getInstance();
										if (serverDriver) {
											LOG(INFO) << "Setting driver into motion compensation mode";
											serverDriver->motionCompensation().setMotionCompensationVelAccMode(message.msg.dm_MotionCompensationMode.velAccCompensationMode);
											info->setMotionCompensationMode();
											_this->_setMotionCompensationMessageId = message.msg.dm_MotionCompensationMode.messageId;
											_this->_setMotionCompensationClientId = message.msg.dm_MotionCompensationMode.clientId;
											resp.status = ipc::ReplyStatus::Ok;
										} else {
											resp.status = ipc::ReplyStatus::UnknownError;
										}
									}
								}
								if (resp.status != ipc::ReplyStatus::Ok) {
									LOG(ERROR) << "Error while updating device pose offset: Error code " << (int)resp.status;
								}
								if (resp.messageId != 0 && resp.status != ipc::ReplyStatus::Ok) {
									_this->sendReply(message.msg.dm_MotionCompensationMode.clientId, resp);
								}
							}
							break;

						case ipc::RequestType::DeviceManipulation_SetMotionCompensationProperties:
							{
								ipc::Reply resp(ipc::ReplyType::GenericReply);
								resp.messageId = message.msg.dm_SetMotionCompensationProperties.messageId;
								auto serverDriver = ServerDriver::getInstance();
								if (serverDriver) {
									LOG(INFO) << "Setting driver motion compensation properties";
									if (message.msg.dm_SetMotionCompensationProperties.velAccCompensationModeValid) {
										serverDriver->motionCompensation().setMotionCompensationVelAccMode(message.msg.dm_SetMotionCompensationProperties.velAccCompensationMode);
									}
									if (message.msg.dm_SetMotionCompensationProperties.kalmanFilterProcessNoiseValid) {
										serverDriver->motionCompensation().setMotionCompensationKalmanProcessVariance(message.msg.dm_SetMotionCompensationProperties.kalmanFilterProcessNoise);
									}
									if (message.msg.dm_SetMotionCompensationProperties.kalmanFilterObservationNoiseValid) {
										serverDriver->motionCompensation().setMotionCompensationKalmanObservationVariance(message.msg.dm_SetMotionCompensationProperties.kalmanFilterObservationNoise);
									}
									if (message.msg.dm_SetMotionCompensationProperties.movingAverageWindowValid) {
										serverDriver->motionCompensation().setMotionCompensationMovingAverageWindow(message.msg.dm_SetMotionCompensationProperties.movingAverageWindow);
									}
									resp.status = ipc::ReplyStatus::Ok;
								} else {
									resp.status = ipc::ReplyStatus::UnknownError;
								}
								if (resp.status != ipc::ReplyStatus::Ok) {
									LOG(ERROR) << "Error while setting motion compensation properties: Error code " << (int)resp.status;
								}
								if (resp.messageId != 0) {
									_this->sendReply(message.msg.dm_SetMotionCompensationProperties.clientId, resp);
								}
							}
							break;

						default:
							LOG(ERROR) << "Error in ipc server receive loop: Unknown message type (" << (int)message.type << ")";
							break;
						}
					} else {
						LOG(ERROR) << "Error in ipc server receive loop: received size is wrong (" << recv_size << " != " << sizeof(ipc::Request) << ")";
					}
				}
			} catch (std::exception& ex) {
				LOG(ERROR) << "Exception caught in ipc server receive loop: " << ex.what();
			}
		}
		boost::interprocess::message_queue::remove(_this->_ipcQueueName.c_str());
	} catch (std::exception& ex) {
		LOG(ERROR) << "Exception caught in ipc server thread: " << ex.what();
	}
	_this->_ipcThreadRunning = false;
	LOG(DEBUG) << "CServerDriver::_ipcThreadFunc: thread stopped";
}


void IpcShmCommunicator::sendReply(uint32_t clientId, const ipc::Reply& reply) {
	std::lock_guard<std::mutex> guard(_sendMutex);
	auto i = _ipcEndpoints.find(clientId);
	if (i != _ipcEndpoints.end()) {
		i->second->send(&reply, sizeof(ipc::Reply), 0);
	} else {
		LOG(ERROR) << "Error while sending reply: Unknown clientId " << clientId;
	}
}


} // end namespace driver
} // end namespace vrinputemulator
