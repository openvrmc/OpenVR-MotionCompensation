#include "driver_ipc_shm.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <openvr_driver.h>
#include <ipc_protocol.h>
#include <openvr_math.h>
#include "../../driver/ServerDriver.h"
#include "../../devicemanipulation/DeviceManipulationHandle.h"


namespace vrmotioncompensation
{
	namespace driver
	{
		void IpcShmCommunicator::init(ServerDriver* driver)
		{
			_driver = driver;
			_ipcThreadStopFlag = false;
			_ipcThread = std::thread(_ipcThreadFunc, this, driver);
		}

		void IpcShmCommunicator::shutdown()
		{
			if (_ipcThreadRunning)
			{
				_ipcThreadStopFlag = true;
				_ipcThread.join();
			}
		}

		void IpcShmCommunicator::_ipcThreadFunc(IpcShmCommunicator* _this, ServerDriver* driver)
		{
			_this->_ipcThreadRunning = true;
			LOG(DEBUG) << "CServerDriver::_ipcThreadFunc: thread started";
			try
			{
		   // Create message queue
				boost::interprocess::message_queue::remove(_this->_ipcQueueName.c_str());
				boost::interprocess::message_queue messageQueue(
					boost::interprocess::create_only,
					_this->_ipcQueueName.c_str(),
					100,					//max message number
					sizeof(ipc::Request)    //max message size
				);

				while (!_this->_ipcThreadStopFlag)
				{
					try
					{
						ipc::Request message;
						uint64_t recv_size;
						unsigned priority;
						boost::posix_time::ptime timeout = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::milliseconds(50);
						if (messageQueue.timed_receive(&message, sizeof(ipc::Request), recv_size, priority, timeout))
						{
							LOG(TRACE) << "CServerDriver::_ipcThreadFunc: IPC request received ( type " << (int)message.type << ")";
							if (recv_size == sizeof(ipc::Request))
							{
								switch (message.type)
								{

								case ipc::RequestType::IPC_ClientConnect:
								{
									try
									{
										auto queue = std::make_shared<boost::interprocess::message_queue>(boost::interprocess::open_only, message.msg.ipc_ClientConnect.queueName);
										ipc::Reply reply(ipc::ReplyType::IPC_ClientConnect);
										reply.messageId = message.msg.ipc_ClientConnect.messageId;
										reply.msg.ipc_ClientConnect.ipcProcotolVersion = IPC_PROTOCOL_VERSION;
										uint32_t clientId = 0;
										if (message.msg.ipc_ClientConnect.ipcProcotolVersion == IPC_PROTOCOL_VERSION)
										{
											clientId = _this->_ipcClientIdNext++;
											_this->_ipcEndpoints.insert({ clientId, queue });
											reply.msg.ipc_ClientConnect.clientId = clientId;
											reply.status = ipc::ReplyStatus::Ok;
											LOG(INFO) << "New client connected: endpoint \"" << message.msg.ipc_ClientConnect.queueName << "\", cliendId " << clientId;
										}
										else
										{
											reply.msg.ipc_ClientConnect.clientId = 0;
											reply.status = ipc::ReplyStatus::InvalidVersion;
											LOG(INFO) << "Client (endpoint \"" << message.msg.ipc_ClientConnect.queueName << "\") reports incompatible ipc version "
												<< message.msg.ipc_ClientConnect.ipcProcotolVersion;
										}
										_this->sendReply(clientId, reply);
									}
									catch (std::exception & e)
									{
										LOG(ERROR) << "Error during client connect: " << e.what();
									}
								}
								break;

								case ipc::RequestType::IPC_ClientDisconnect:
								{
									ipc::Reply reply(ipc::ReplyType::GenericReply);
									reply.messageId = message.msg.ipc_ClientDisconnect.messageId;
									auto i = _this->_ipcEndpoints.find(message.msg.ipc_ClientDisconnect.clientId);
									if (i != _this->_ipcEndpoints.end())
									{
										reply.status = ipc::ReplyStatus::Ok;
										LOG(INFO) << "Client disconnected: clientId " << message.msg.ipc_ClientDisconnect.clientId;
										if (reply.messageId != 0)
										{
											_this->sendReply(message.msg.ipc_ClientDisconnect.clientId, reply);
										}
										_this->_ipcEndpoints.erase(i);
									}
									else
									{
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

								case ipc::RequestType::DeviceManipulation_GetDeviceInfo:
								{
									ipc::Reply resp(ipc::ReplyType::GenericReply);
									resp.messageId = message.msg.ovr_GenericDeviceIdMessage.messageId;

									if (message.msg.ovr_GenericDeviceIdMessage.OpenVRId >= vr::k_unMaxTrackedDeviceCount)
									{
										resp.status = ipc::ReplyStatus::InvalidId;
									}
									else
									{
										DeviceManipulationHandle* info = driver->getDeviceManipulationHandleById(message.msg.ovr_GenericDeviceIdMessage.OpenVRId);
										if (!info)
										{
											resp.status = ipc::ReplyStatus::NotFound;
											resp.msg.dm_deviceInfo.deviceClass = vr::ETrackedDeviceClass::TrackedDeviceClass_Invalid;
										}
										else
										{
											resp.status = ipc::ReplyStatus::Ok;
											resp.msg.dm_deviceInfo.OpenVRId = message.msg.ovr_GenericDeviceIdMessage.OpenVRId;
											resp.msg.dm_deviceInfo.deviceMode = info->getDeviceMode();
											resp.msg.dm_deviceInfo.deviceClass = info->deviceClass();
										}
									}

									/*if (resp.status != ipc::ReplyStatus::Ok)
									{
										LOG(ERROR) << "Error while getting device info: Error code " << (int)resp.status;
									}*/

									if (resp.messageId != 0)
									{
										_this->sendReply(message.msg.ovr_GenericDeviceIdMessage.clientId, resp);
									}
								}
								break;

								case ipc::RequestType::DeviceManipulation_MotionCompensationMode:
								{
									// Create reply message
									ipc::Reply resp(ipc::ReplyType::GenericReply);
									resp.messageId = message.msg.dm_MotionCompensationMode.messageId;

									if (message.msg.dm_MotionCompensationMode.MCdeviceId > vr::k_unMaxTrackedDeviceCount ||
										(message.msg.dm_MotionCompensationMode.RTdeviceId > vr::k_unMaxTrackedDeviceCount && message.msg.dm_MotionCompensationMode.CompensationMode == MotionCompensationMode::ReferenceTracker))
									{
										resp.status = ipc::ReplyStatus::InvalidId;
									}
									else
									{
										DeviceManipulationHandle* MCdevice = driver->getDeviceManipulationHandleById(message.msg.dm_MotionCompensationMode.MCdeviceId);
										DeviceManipulationHandle* RTdevice = driver->getDeviceManipulationHandleById(message.msg.dm_MotionCompensationMode.RTdeviceId);

										int MCdeviceID = message.msg.dm_MotionCompensationMode.MCdeviceId;
										int RTdeviceID = message.msg.dm_MotionCompensationMode.RTdeviceId;

										if (!MCdevice || !RTdevice)
										{
											resp.status = ipc::ReplyStatus::NotFound;
										}
										else
										{
											auto serverDriver = ServerDriver::getInstance();

											if (serverDriver)
											{
												if (message.msg.dm_MotionCompensationMode.CompensationMode == MotionCompensationMode::ReferenceTracker)
												{
													LOG(INFO) << "Setting driver into motion compensation mode";

													//Check if an old device needs a mode change
													if (serverDriver->motionCompensation().getMotionCompensationMode() == MotionCompensationMode::ReferenceTracker)
													{
														//New MCdevice is different from old
														if (serverDriver->motionCompensation().getMCdeviceID() != MCdeviceID)
														{
															//Set old MCdevice to default
															DeviceManipulationHandle* OldMCdevice = driver->getDeviceManipulationHandleById(serverDriver->motionCompensation().getMCdeviceID());
															OldMCdevice->setMotionCompensationDeviceMode(MotionCompensationDeviceMode::Default);

															//Set new MCdevice to motion compensated
															MCdevice->setMotionCompensationDeviceMode(MotionCompensationDeviceMode::MotionCompensated);
															serverDriver->motionCompensation().setNewReferenceTracker(MCdeviceID);
														}

														//New RTdevice is different from old
														if (serverDriver->motionCompensation().getRTdeviceID() != RTdeviceID)
														{
															//Set old RTdevice to default
															DeviceManipulationHandle* OldRTdevice = driver->getDeviceManipulationHandleById(serverDriver->motionCompensation().getRTdeviceID());
															OldRTdevice->setMotionCompensationDeviceMode(MotionCompensationDeviceMode::Default);

															//Set new RTdevice to reference tracker
															RTdevice->setMotionCompensationDeviceMode(MotionCompensationDeviceMode::ReferenceTracker);
															serverDriver->motionCompensation().setNewReferenceTracker(RTdeviceID);
														}
													}
													else
													{
														//Activate motion compensation mode for specified device
														MCdevice->setMotionCompensationDeviceMode(MotionCompensationDeviceMode::MotionCompensated);
														RTdevice->setMotionCompensationDeviceMode(MotionCompensationDeviceMode::ReferenceTracker);

														//Set motion compensation mode
														serverDriver->motionCompensation().setMotionCompensationMode(MotionCompensationMode::ReferenceTracker, MCdeviceID, RTdeviceID);
													}
												}
												else if (message.msg.dm_MotionCompensationMode.CompensationMode == MotionCompensationMode::Disabled)
												{
													LOG(INFO) << "Setting driver into default mode";

													MCdevice->setMotionCompensationDeviceMode(MotionCompensationDeviceMode::Default);
													RTdevice->setMotionCompensationDeviceMode(MotionCompensationDeviceMode::Default);

													//Reset and set some vars for every device
													serverDriver->motionCompensation().setMotionCompensationMode(MotionCompensationMode::Disabled, -1, -1);
												}

												resp.status = ipc::ReplyStatus::Ok;
											}
											else
											{
												resp.status = ipc::ReplyStatus::UnknownError;
											}
										}
									}

									if (resp.status != ipc::ReplyStatus::Ok)
									{
										LOG(ERROR) << "Error while setting device into motion compensation mode: Error code " << (int)resp.status;
										LOG(ERROR) << "MCdeviceID: " << message.msg.dm_MotionCompensationMode.MCdeviceId << ", RTdeviceID: " << message.msg.dm_MotionCompensationMode.RTdeviceId;
									}

									if (resp.messageId != 0/* && resp.status != ipc::ReplyStatus::Ok*/)
									{
										_this->sendReply(message.msg.dm_MotionCompensationMode.clientId, resp);
									}
								}
								break;

								case ipc::RequestType::DeviceManipulation_SetMotionCompensationProperties:
								{
									ipc::Reply resp(ipc::ReplyType::GenericReply);
									resp.messageId = message.msg.dm_SetMotionCompensationProperties.messageId;
									auto serverDriver = ServerDriver::getInstance();
									if (serverDriver)
									{
										LOG(INFO) << "Setting driver motion compensation properties:";
										LOG(INFO) << "LPF_Beta: " << message.msg.dm_SetMotionCompensationProperties.LPFBeta;
										LOG(INFO) << "samples: " << message.msg.dm_SetMotionCompensationProperties.samples;
										LOG(INFO) << "set Zero: " << message.msg.dm_SetMotionCompensationProperties.setZero;
										LOG(INFO) << "offset X: " << message.msg.dm_SetMotionCompensationProperties.offsets.v[0] 
													   << ", Y: " << message.msg.dm_SetMotionCompensationProperties.offsets.v[1]
													   << ", Z: " << message.msg.dm_SetMotionCompensationProperties.offsets.v[2];
										LOG(INFO) << "End of property listing";

										serverDriver->motionCompensation().setLpfBeta(message.msg.dm_SetMotionCompensationProperties.LPFBeta);
										serverDriver->motionCompensation().setAlpha(message.msg.dm_SetMotionCompensationProperties.samples);
										serverDriver->motionCompensation().setZeroMode(message.msg.dm_SetMotionCompensationProperties.setZero);
										serverDriver->motionCompensation().setOffsets(message.msg.dm_SetMotionCompensationProperties.offsets);

										resp.status = ipc::ReplyStatus::Ok;
									}
									else
									{
										resp.status = ipc::ReplyStatus::UnknownError;
									}

									if (resp.status != ipc::ReplyStatus::Ok)
									{
										LOG(ERROR) << "Error while setting motion compensation properties: Error code " << (int)resp.status;
									}

									if (resp.messageId != 0)
									{
										_this->sendReply(message.msg.dm_SetMotionCompensationProperties.clientId, resp);
									}
								}
								break;

								case ipc::RequestType::DeviceManipulation_ResetRefZeroPose:
								{
									ipc::Reply resp(ipc::ReplyType::GenericReply);
									resp.messageId = message.msg.dm_SetMotionCompensationProperties.messageId;
									auto serverDriver = ServerDriver::getInstance();
									if (serverDriver)
									{
										LOG(INFO) << "Resetting reference zero pose";

										serverDriver->motionCompensation().resetZeroPose();

										resp.status = ipc::ReplyStatus::Ok;
									}
									else
									{
										resp.status = ipc::ReplyStatus::UnknownError;
									}

									if (resp.status != ipc::ReplyStatus::Ok)
									{
										LOG(ERROR) << "Error while setting motion compensation properties: Error code " << (int)resp.status;
									}

									if (resp.messageId != 0)
									{
										_this->sendReply(message.msg.dm_SetMotionCompensationProperties.clientId, resp);
									}
								}
								break;

								case ipc::RequestType::DebugLogger_Settings:
								{
									ipc::Reply resp(ipc::ReplyType::GenericReply);
									resp.messageId = message.msg.dl_Settings.messageId;
									auto serverDriver = ServerDriver::getInstance();
									if (serverDriver)
									{
										if (message.msg.dl_Settings.enabled)
										{
											if (!serverDriver->motionCompensation().StartDebugData())
											{
												LOG(INFO) << "Could not start debug logger: Motion Compensation must be enabled";
												resp.status = ipc::ReplyStatus::InvalidId;
											}
											else
											{
												LOG(INFO) << "Debug logger enabled";
												LOG(INFO) << "Max debug data points = " << message.msg.dl_Settings.MaxDebugPoints;
												resp.status = ipc::ReplyStatus::Ok;
											}	
										}
										else
										{
											LOG(INFO) << "Debug logger disabled";
											serverDriver->motionCompensation().StopDebugData();
											resp.status = ipc::ReplyStatus::Ok;
										}
									}
									else
									{
										resp.status = ipc::ReplyStatus::UnknownError;
									}

									if (resp.status != ipc::ReplyStatus::Ok)
									{
										LOG(ERROR) << "Error while starting debug logger: Error code " << (int)resp.status;
									}

									if (resp.messageId != 0)
									{
										_this->sendReply(message.msg.dl_Settings.clientId, resp);
									}
								}
								break;

								default:
									LOG(ERROR) << "Error in ipc server receive loop: Unknown message type (" << (int)message.type << ")";
									break;
								}
							}
							else
							{
								LOG(ERROR) << "Error in ipc server receive loop: received size is wrong (" << recv_size << " != " << sizeof(ipc::Request) << ")";
							}
						}
					}
					catch (std::exception & ex)
					{
						LOG(ERROR) << "Exception caught in ipc server receive loop: " << ex.what();
					}
				}
				boost::interprocess::message_queue::remove(_this->_ipcQueueName.c_str());
			}
			catch (std::exception & ex)
			{
				LOG(ERROR) << "Exception caught in ipc server thread: " << ex.what();
			}
			_this->_ipcThreadRunning = false;
			LOG(DEBUG) << "CServerDriver::_ipcThreadFunc: thread stopped";
		}

		void IpcShmCommunicator::sendReply(uint32_t clientId, const ipc::Reply& reply)
		{
			std::lock_guard<std::mutex> guard(_sendMutex);
			auto i = _ipcEndpoints.find(clientId);
			if (i != _ipcEndpoints.end())
			{
				i->second->send(&reply, sizeof(ipc::Reply), 0);
			}
			else
			{
				LOG(ERROR) << "Error while sending reply: Unknown clientId " << clientId;
			}
		}

	} // end namespace driver
} // end namespace vrmotioncompensation
