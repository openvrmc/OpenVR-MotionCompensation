#pragma once

#include "vrmotioncompensation_types.h"
#include <utility>


#define IPC_PROTOCOL_VERSION 3

namespace vrmotioncompensation
{
	namespace ipc
	{
		enum class RequestType : uint32_t
		{
			None,

			// IPC connection handling
			IPC_ClientConnect,
			IPC_ClientDisconnect,
			IPC_Ping,

			DeviceManipulation_GetDeviceInfo,
			DeviceManipulation_MotionCompensationMode,
			DeviceManipulation_SetMotionCompensationProperties

		};

		enum class ReplyType : uint32_t
		{
			None,

			IPC_ClientConnect,
			IPC_Ping,

			GenericReply,

			DeviceManipulation_GetDeviceInfo

		};

		enum class ReplyStatus : uint32_t
		{
			None,
			Ok,
			UnknownError,
			InvalidId,
			AlreadyInUse,
			InvalidType,
			NotFound,
			TooManyDevices,
			InvalidVersion,
			MissingProperty,
			InvalidOperation,
			NotTracking
		};

		struct Request_IPC_ClientConnect
		{
			uint32_t messageId;
			uint32_t ipcProcotolVersion;
			char queueName[128];
		};

		struct Request_IPC_ClientDisconnect
		{
			uint32_t clientId;
			uint32_t messageId;
		};

		struct Request_IPC_Ping
		{
			uint32_t clientId;
			uint32_t messageId;
			uint64_t nonce;
		};

		struct Request_OpenVR_GenericClientMessage
		{
			uint32_t clientId;
			uint32_t messageId; // Used to associate with Reply
		};

		struct Request_OpenVR_GenericDeviceIdMessage
		{
			uint32_t clientId;
			uint32_t messageId; // Used to associate with Reply
			uint32_t deviceId;
		};

		struct Request_DeviceManipulation_MotionCompensationMode
		{
			uint32_t clientId;
			uint32_t messageId;			// Used to associate with Reply
			uint32_t MCdeviceId;		// Motion compensated device ID
			uint32_t RTdeviceId;		// Reference tracker device ID
			MotionCompensationMode CompensationMode;
		};

		struct Request_DeviceManipulation_SetMotionCompensationProperties
		{
			uint32_t clientId;
			uint32_t messageId; // Used to associate with Reply
			double LPFBeta;
		};

		struct Request
		{
			Request()
			{
			}
			Request(RequestType type) : type(type)
			{
				timestamp = std::chrono::duration_cast <std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			}
			Request(RequestType type, uint64_t timestamp) : type(type), timestamp(timestamp)
			{
			}

			void refreshTimestamp()
			{
				timestamp = std::chrono::duration_cast <std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			}

			RequestType type = RequestType::None;
			int64_t timestamp = 0; // milliseconds since epoch
			union MsgUnion
			{
				Request_IPC_ClientConnect ipc_ClientConnect;
				Request_IPC_ClientDisconnect ipc_ClientDisconnect;
				Request_IPC_Ping ipc_Ping;
				Request_OpenVR_GenericClientMessage ovr_GenericClientMessage;
				Request_OpenVR_GenericDeviceIdMessage ovr_GenericDeviceIdMessage;
				Request_DeviceManipulation_MotionCompensationMode dm_MotionCompensationMode;
				Request_DeviceManipulation_SetMotionCompensationProperties dm_SetMotionCompensationProperties;
				MsgUnion()
				{
				}
			} msg;
		};

		struct Reply_IPC_ClientConnect
		{
			uint32_t clientId;
			uint32_t ipcProcotolVersion;
		};

		struct Reply_IPC_Ping
		{
			uint64_t nonce;
		};

		struct Reply_DeviceManipulation_GetDeviceInfo
		{
			uint32_t deviceId;
			vr::ETrackedDeviceClass deviceClass;
			MotionCompensationDeviceMode deviceMode;
		};

		struct Reply
		{
			Reply()
			{
			}
			Reply(ReplyType type) : type(type)
			{
				timestamp = std::chrono::duration_cast <std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			}
			Reply(ReplyType type, uint64_t timestamp) : type(type), timestamp(timestamp)
			{
			}

			ReplyType type = ReplyType::None;
			uint64_t timestamp = 0; // milliseconds since epoch
			uint32_t messageId;
			ReplyStatus status;
			union MsgUnion
			{
				Reply_IPC_ClientConnect ipc_ClientConnect;
				Reply_IPC_Ping ipc_Ping;
				Reply_DeviceManipulation_GetDeviceInfo dm_deviceInfo;
				MsgUnion()
				{
				}
			} msg;
		};

	} // end namespace ipc
} // end namespace vrmotioncompensation
