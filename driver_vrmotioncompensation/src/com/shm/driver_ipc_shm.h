#pragma once

#include <thread>
#include <string>
#include <map>
#include <mutex>
#include <memory>
#include <boost/interprocess/ipc/message_queue.hpp>


// driver namespace
namespace vrmotioncompensation
{
	// forward declarations
	namespace ipc
	{
		struct Reply;
	}

	namespace driver
	{
		// forward declarations
		class ServerDriver;

		// This class handles setting up the server and receiving messages from the client (the SteamVR overlay).
		// Mainly, it receives a request for information about an OpenVR device, where it passes along the DeviceManipulationHandle
		// OR, it recieves a request to set MotionCompensation to true. In this case, it sets the flag in the DeviceManipulationHandle for the
		//	MC and RT devices, and it forwards the OpenVR IDs to the MotionCompensationManager that is attached to the ServerDriver.
		// I'm guessing the MotionCompensationManager then does it's magic on the set device if the MotionCompensationMode flag is enabled there.
		class IpcShmCommunicator
		{
		public:
			void init(ServerDriver* driver);
			void shutdown();

		private:
			static void _ipcThreadFunc(IpcShmCommunicator* _this, ServerDriver* driver);

			void sendReply(uint32_t clientId, const ipc::Reply& reply);

			std::mutex _sendMutex;
			ServerDriver* _driver = nullptr;
			std::thread _ipcThread;
			volatile bool _ipcThreadRunning = false;
			volatile bool _ipcThreadStopFlag = false;
			std::string _ipcQueueName = "driver_vrmotioncompensation.server_queue";
			uint32_t _ipcClientIdNext = 1;
			std::map<uint32_t, std::shared_ptr<boost::interprocess::message_queue>> _ipcEndpoints;

			// This is not exactly multi-user safe, maybe I fix it in the future
			uint32_t _setMotionCompensationClientId = 0;
			uint32_t _setMotionCompensationMessageId = 0;
		};
	} // end namespace driver
} // end namespace vrmotioncompensation
