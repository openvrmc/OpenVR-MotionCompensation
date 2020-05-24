#include <vrmotioncompensation.h>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <config.h>


#if VRMOTIONCOMPENSATION_EASYLOGGING == 1
#include "logging.h";
#define WRITELOG(level, txt) LOG(level) << txt;
#else
#define WRITELOG(level, txt) std::cerr << txt;
#endif



namespace vrmotioncompensation
{
// Receives and dispatches ipc messages
	void VRMotionCompensation::_ipcThreadFunc(VRMotionCompensation* _this)
	{
		_this->_ipcThreadRunning = true;
		while (!_this->_ipcThreadStop)
		{
			try
			{
				ipc::Reply message;
				uint64_t recv_size;
				unsigned priority;
				boost::posix_time::ptime timeout = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::milliseconds(50);
				if (_this->_ipcClientQueue->timed_receive(&message, sizeof(ipc::Reply), recv_size, priority, timeout))
				{
					if (recv_size == sizeof(ipc::Reply))
					{
						std::lock_guard<std::recursive_mutex> lock(_this->_mutex);
						auto i = _this->_ipcPromiseMap.find(message.messageId);
						if (i != _this->_ipcPromiseMap.end())
						{
							if (i->second.isValid)
							{
								i->second.promise.set_value(message);
							}
							else
							{
								_this->_ipcPromiseMap.erase(i); // nobody wants it, so we delete it
							}
						}
					}
				}
				else
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
			}
			catch (std::exception & ex)
			{
				WRITELOG(ERROR, "Exception in ipc receive loop: " << ex.what() << std::endl);
			}
		}
		_this->_ipcThreadRunning = false;
	}

	VRMotionCompensation::VRMotionCompensation(const std::string& serverQueue, const std::string& clientQueue) : _ipcServerQueueName(serverQueue), _ipcClientQueueName(clientQueue)
	{
	}

	VRMotionCompensation::~VRMotionCompensation()
	{
		disconnect();
	}

	bool VRMotionCompensation::isConnected() const
	{
		return _ipcServerQueue != nullptr;
	}

	void VRMotionCompensation::connect()
	{
		if (!_ipcServerQueue)
		{
		// Open server-side message queue
			try
			{
				_ipcServerQueue = new boost::interprocess::message_queue(boost::interprocess::open_only, _ipcServerQueueName.c_str());
			}
			catch (std::exception & e)
			{
				_ipcServerQueue = nullptr;
				std::stringstream ss;
				ss << "Could not open server-side message queue: " << e.what();
				throw vrmotioncompensation_connectionerror(ss.str());
			}
			// Append random number to client queue name (and hopefully no other client uses the same random number)
			_ipcClientQueueName += std::to_string(_ipcRandomDist(_ipcRandomDevice));
			// Open client-side message queue
			try
			{
				boost::interprocess::message_queue::remove(_ipcClientQueueName.c_str());
				_ipcClientQueue = new boost::interprocess::message_queue(
					boost::interprocess::create_only,
					_ipcClientQueueName.c_str(),
					100,					//max message number
					sizeof(ipc::Reply)    //max message size
				);
			}
			catch (std::exception & e)
			{
				delete _ipcServerQueue;
				_ipcServerQueue = nullptr;
				_ipcClientQueue = nullptr;
				std::stringstream ss;
				ss << "Could not open client-side message queue: " << e.what();
				throw vrmotioncompensation_connectionerror(ss.str());
			}
			// Start ipc thread
			_ipcThreadStop = false;
			_ipcThread = std::thread(_ipcThreadFunc, this);
			// Send ClientConnect message to server
			ipc::Request message(ipc::RequestType::IPC_ClientConnect);
			auto messageId = _ipcRandomDist(_ipcRandomDevice);
			message.msg.ipc_ClientConnect.messageId = messageId;
			message.msg.ipc_ClientConnect.ipcProcotolVersion = IPC_PROTOCOL_VERSION;
			strncpy_s(message.msg.ipc_ClientConnect.queueName, _ipcClientQueueName.c_str(), 127);
			message.msg.ipc_ClientConnect.queueName[127] = '\0';
			std::promise<ipc::Reply> respPromise;
			auto respFuture = respPromise.get_future();
			{
				std::lock_guard<std::recursive_mutex> lock(_mutex);
				_ipcPromiseMap.insert({ messageId, std::move(respPromise) });
			}
			_ipcServerQueue->send(&message, sizeof(ipc::Request), 0);
			// Wait for response
			auto resp = respFuture.get();
			m_clientId = resp.msg.ipc_ClientConnect.clientId;
			{
				std::lock_guard<std::recursive_mutex> lock(_mutex);
				_ipcPromiseMap.erase(messageId);
			}
			if (resp.status != ipc::ReplyStatus::Ok)
			{
				delete _ipcServerQueue;
				_ipcServerQueue = nullptr;
				delete _ipcClientQueue;
				_ipcClientQueue = nullptr;
				std::stringstream ss;
				ss << "Connection rejected by server: ";
				if (resp.status == ipc::ReplyStatus::InvalidVersion)
				{
					ss << "Incompatible ipc protocol versions (server: " << resp.msg.ipc_ClientConnect.ipcProcotolVersion << ", client: " << IPC_PROTOCOL_VERSION << ")";
					throw vrmotioncompensation_invalidversion(ss.str());
				}
				else if (resp.status != ipc::ReplyStatus::Ok)
				{
					ss << "Error code " << (int)resp.status;
					throw vrmotioncompensation_connectionerror(ss.str());
				}
			}
		}
	}

	void VRMotionCompensation::disconnect()
	{
		if (_ipcServerQueue)
		{
			// Send disconnect message (so the server can free resources)
			ipc::Request message(ipc::RequestType::IPC_ClientDisconnect);
			auto messageId = _ipcRandomDist(_ipcRandomDevice);
			message.msg.ipc_ClientDisconnect.clientId = m_clientId;
			message.msg.ipc_ClientDisconnect.messageId = messageId;
			std::promise<ipc::Reply> respPromise;
			auto respFuture = respPromise.get_future();
			{
				std::lock_guard<std::recursive_mutex> lock(_mutex);
				_ipcPromiseMap.insert({ messageId, std::move(respPromise) });
			}
			_ipcServerQueue->send(&message, sizeof(ipc::Request), 0);
			auto resp = respFuture.get();
			m_clientId = resp.msg.ipc_ClientConnect.clientId;
			{
				std::lock_guard<std::recursive_mutex> lock(_mutex);
				_ipcPromiseMap.erase(messageId);
			}
			// Stop ipc thread
			if (_ipcThreadRunning)
			{
				_ipcThreadStop = true;
				_ipcThread.join();
			}
			// delete message queues
			if (_ipcServerQueue)
			{
				delete _ipcServerQueue;
				_ipcServerQueue = nullptr;
			}
			if (_ipcClientQueue)
			{
				delete _ipcClientQueue;
				_ipcClientQueue = nullptr;
			}
		}
	}

	void VRMotionCompensation::ping(bool modal, bool enableReply)
	{
		if (_ipcServerQueue)
		{
			uint32_t messageId = _ipcRandomDist(_ipcRandomDevice);
			uint64_t nonce = _ipcRandomDist(_ipcRandomDevice);
			ipc::Request message(ipc::RequestType::IPC_Ping);
			message.msg.ipc_Ping.clientId = m_clientId;
			message.msg.ipc_Ping.messageId = messageId;
			message.msg.ipc_Ping.nonce = nonce;
			if (modal)
			{
				std::promise<ipc::Reply> respPromise;
				auto respFuture = respPromise.get_future();
				{
					std::lock_guard<std::recursive_mutex> lock(_mutex);
					_ipcPromiseMap.insert({ messageId, std::move(respPromise) });
				}
				_ipcServerQueue->send(&message, sizeof(ipc::Request), 0);
				auto resp = respFuture.get();
				{
					std::lock_guard<std::recursive_mutex> lock(_mutex);
					_ipcPromiseMap.erase(messageId);
				}
				if (resp.status != ipc::ReplyStatus::Ok)
				{
					std::stringstream ss;
					ss << "Error while pinging server: Error code " << (int)resp.status;
					throw vrmotioncompensation_exception(ss.str());
				}
			}
			else
			{
				if (enableReply)
				{
					std::lock_guard<std::recursive_mutex> lock(_mutex);
					message.msg.ipc_Ping.messageId = messageId;
					_ipcPromiseMap.insert({ messageId, _ipcPromiseMapEntry() });
				}
				else
				{
					message.msg.ipc_Ping.messageId = 0;
				}
				_ipcServerQueue->send(&message, sizeof(ipc::Request), 0);
			}
		}
		else
		{
			throw vrmotioncompensation_connectionerror("No active connection.");
		}
	}

	void VRMotionCompensation::getDeviceInfo(uint32_t OpenVRId, DeviceInfo& info)
	{
		if (_ipcServerQueue)
		{
			//Create message
			ipc::Request message(ipc::RequestType::DeviceManipulation_GetDeviceInfo);			
			memset(&message.msg, 0, sizeof(message.msg));
			message.msg.ovr_GenericDeviceIdMessage.clientId = m_clientId;
			message.msg.ovr_GenericDeviceIdMessage.OpenVRId = OpenVRId;

			//Create random message ID
			uint32_t messageId = _ipcRandomDist(_ipcRandomDevice);
			message.msg.ovr_GenericDeviceIdMessage.messageId = messageId;

			//Allocate memory for the reply
			std::promise<ipc::Reply> respPromise;
			auto respFuture = respPromise.get_future();
			{
				std::lock_guard<std::recursive_mutex> lock(_mutex);
				_ipcPromiseMap.insert({ messageId, std::move(respPromise) });
			}

			//Send message
			_ipcServerQueue->send(&message, sizeof(ipc::Request), 0);

			auto resp = respFuture.get();
			{
				std::lock_guard<std::recursive_mutex> lock(_mutex);
				_ipcPromiseMap.erase(messageId);
			}

			//If there was an error, notify the user
			std::stringstream ss;
			ss << "Error while getting device info: ";

			if (resp.status == ipc::ReplyStatus::Ok)
			{
				info.OpenVRId = resp.msg.dm_deviceInfo.OpenVRId;
				info.deviceClass = resp.msg.dm_deviceInfo.deviceClass;
				info.deviceMode = resp.msg.dm_deviceInfo.deviceMode;
			}
			else if (resp.status == ipc::ReplyStatus::NotFound)
			{
				info.deviceClass = resp.msg.dm_deviceInfo.deviceClass;
			}
			else if (resp.status == ipc::ReplyStatus::InvalidId)
			{
				ss << "Invalid device id";
				throw vrmotioncompensation_invalidid(ss.str());
			}
			/*else if (resp.status == ipc::ReplyStatus::NotFound)
			{
				ss << "Device not found";
				throw vrmotioncompensation_notfound(ss.str());
			}*/
			else if (resp.status != ipc::ReplyStatus::Ok)
			{
				ss << "Error code " << (int)resp.status;
				throw vrmotioncompensation_exception(ss.str());
			}
		}
		else
		{
			throw vrmotioncompensation_connectionerror("No active connection.");
		}
	}

	void VRMotionCompensation::setDeviceMotionCompensationMode(uint32_t MCdeviceId, uint32_t RTdeviceId, MotionCompensationMode Mode, bool modal)
	{
		if (_ipcServerQueue)
		{
			//Create message
			ipc::Request message(ipc::RequestType::DeviceManipulation_MotionCompensationMode);
			memset(&message.msg, 0, sizeof(message.msg));
			message.msg.dm_MotionCompensationMode.clientId = m_clientId;
			message.msg.dm_MotionCompensationMode.messageId = 0;
			message.msg.dm_MotionCompensationMode.MCdeviceId = MCdeviceId;
			message.msg.dm_MotionCompensationMode.RTdeviceId = RTdeviceId;
			message.msg.dm_MotionCompensationMode.CompensationMode = Mode;

			if (modal)
			{
				//Create random message ID
				uint32_t messageId = _ipcRandomDist(_ipcRandomDevice);
				message.msg.dm_MotionCompensationMode.messageId = messageId;

				//Allocate memory for the reply
				std::promise<ipc::Reply> respPromise;
				auto respFuture = respPromise.get_future();
				{
					std::lock_guard<std::recursive_mutex> lock(_mutex);
					_ipcPromiseMap.insert({ messageId, std::move(respPromise) });
				}

				//Send message
				_ipcServerQueue->send(&message, sizeof(ipc::Request), 0);

				auto resp = respFuture.get();
				{
					std::lock_guard<std::recursive_mutex> lock(_mutex);
					_ipcPromiseMap.erase(messageId);
				}

				//If there was an error, notify the user
				std::stringstream ss;
				ss << "Error while setting motion compensation mode: ";

				if (resp.status == ipc::ReplyStatus::InvalidId)
				{
					ss << "Invalid device id";
					throw vrmotioncompensation_invalidid(ss.str(), (int)resp.status);
				}
				else if (resp.status == ipc::ReplyStatus::NotFound)
				{
					ss << "Device not found";
					throw vrmotioncompensation_notfound(ss.str(), (int)resp.status);
				}
				else if (resp.status == ipc::ReplyStatus::SharedMemoryError)
				{
					ss << "MMF could not be opened";
					throw vrmotioncompensation_sharedmemoryerror(ss.str(), (int)resp.status);
				}
				else if (resp.status != ipc::ReplyStatus::Ok)
				{
					ss << "Error code " << (int)resp.status;
					throw vrmotioncompensation_exception(ss.str(), (int)resp.status);
				}
			}
			else
			{
				_ipcServerQueue->send(&message, sizeof(ipc::Request), 0);
			}
		}
		else
		{
			throw vrmotioncompensation_connectionerror("No active connection.");
		}
	}

	void VRMotionCompensation::setMoticonCompensationSettings(double LPF_Beta, uint32_t samples, bool setZero, vr::HmdVector3d_t offsets)
	{
		if (_ipcServerQueue)
		{
			//Create message
			ipc::Request message(ipc::RequestType::DeviceManipulation_SetMotionCompensationProperties);
			memset(&message.msg, 0, sizeof(message.msg));
			message.msg.dm_SetMotionCompensationProperties.clientId = m_clientId;
			message.msg.dm_SetMotionCompensationProperties.messageId = 0;
			message.msg.dm_SetMotionCompensationProperties.LPFBeta = LPF_Beta;
			message.msg.dm_SetMotionCompensationProperties.samples = samples;
			message.msg.dm_SetMotionCompensationProperties.setZero = setZero;
			message.msg.dm_SetMotionCompensationProperties.offsets = offsets;


			//Create random message ID
			uint32_t messageId = _ipcRandomDist(_ipcRandomDevice);
			message.msg.dm_SetMotionCompensationProperties.messageId = messageId;

			//Allocate memory for the reply
			std::promise<ipc::Reply> respPromise;
			auto respFuture = respPromise.get_future();
			{
				std::lock_guard<std::recursive_mutex> lock(_mutex);
				_ipcPromiseMap.insert({ messageId, std::move(respPromise) });
			}

			//Send message
			_ipcServerQueue->send(&message, sizeof(ipc::Request), 0);
			WRITELOG(INFO, "MC message created sending to driver" << std::endl);

			auto resp = respFuture.get();
			{
				std::lock_guard<std::recursive_mutex> lock(_mutex);
				_ipcPromiseMap.erase(messageId);
			}

			//If there was an error, notify the user
			std::stringstream ss;
			ss << "Error while setting motion compensation mode: ";

			if (resp.status != ipc::ReplyStatus::Ok)
			{
				ss << "Error code " << (int)resp.status;
				throw vrmotioncompensation_exception(ss.str(), (int)resp.status);
			}
		}
		else
		{
			throw vrmotioncompensation_connectionerror("No active connection.");
		}
	}

	void VRMotionCompensation::startDebugLogger(bool enable, bool modal)
	{
		if (_ipcServerQueue)
		{
			//Create message
			ipc::Request message(ipc::RequestType::DebugLogger_Settings);
			memset(&message.msg, 0, sizeof(message.msg));
			message.msg.dl_Settings.clientId = m_clientId;
			message.msg.dl_Settings.messageId = 0;
			message.msg.dl_Settings.enabled = enable;

			if (modal)
			{
				//Create random message ID
				uint32_t messageId = _ipcRandomDist(_ipcRandomDevice);
				message.msg.dl_Settings.messageId = messageId;

				//Allocate memory for the reply
				std::promise<ipc::Reply> respPromise;
				auto respFuture = respPromise.get_future();
				{
					std::lock_guard<std::recursive_mutex> lock(_mutex);
					_ipcPromiseMap.insert({ messageId, std::move(respPromise) });
				}

				//Send message
				_ipcServerQueue->send(&message, sizeof(ipc::Request), 0);
				WRITELOG(INFO, "DL message created sending to driver" << std::endl);

				auto resp = respFuture.get();
				{
					std::lock_guard<std::recursive_mutex> lock(_mutex);
					_ipcPromiseMap.erase(messageId);
				}

				//If there was an error, notify the user
				std::stringstream ss;
				ss << "Error while starting debug logger: ";

				if (resp.status == ipc::ReplyStatus::InvalidId)
				{
					ss << "MC must be running";
					throw vrmotioncompensation_invalidid(ss.str(), (int)resp.status);
				}
				else if (resp.status != ipc::ReplyStatus::Ok)
				{
					ss << "Error code " << (int)resp.status;
					throw vrmotioncompensation_exception(ss.str(), (int)resp.status);
				}
			}
			else
			{
				_ipcServerQueue->send(&message, sizeof(ipc::Request), 0);
				WRITELOG(INFO, "DL message created sending to driver" << std::endl);
			}
		}
		else
		{
			throw vrmotioncompensation_connectionerror("No active connection.");
		}
	}
} // end namespace vrmotioncompensation