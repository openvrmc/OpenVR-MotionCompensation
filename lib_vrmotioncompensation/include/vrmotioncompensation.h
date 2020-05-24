#pragma once

#include <stdint.h>
#include <string>
#include <future>
#include <mutex>
#include <thread>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <openvr.h>
#include <boost/interprocess/ipc/message_queue.hpp>


namespace vr
{
	// Copied from openvr_driver.h
	struct DriverPose_t
	{
		/* Time offset of this pose, in seconds from the actual time of the pose,
		* relative to the time of the PoseUpdated() call made by the driver.
		*/
		double poseTimeOffset;

		/* Generally, the pose maintained by a driver
		* is in an inertial coordinate system different
		* from the world system of x+ right, y+ up, z+ back.
		* Also, the driver is not usually tracking the "head" position,
		* but instead an internal IMU or another reference point in the HMD.
		* The following two transforms transform positions and orientations
		* to app world space from driver world space,
		* and to HMD head space from driver local body space.
		*
		* We maintain the driver pose state in its internal coordinate system,
		* so we can do the pose prediction math without having to
		* use angular acceleration.  A driver's angular acceleration is generally not measured,
		* and is instead calculated from successive samples of angular velocity.
		* This leads to a noisy angular acceleration values, which are also
		* lagged due to the filtering required to reduce noise to an acceptable level.
		*/
		vr::HmdQuaternion_t qWorldFromDriverRotation;
		double vecWorldFromDriverTranslation[3];

		vr::HmdQuaternion_t qDriverFromHeadRotation;
		double vecDriverFromHeadTranslation[3];

		/* State of driver pose, in meters and radians. */
		/* Position of the driver tracking reference in driver world space
		* +[0] (x) is right
		* +[1] (y) is up
		* -[2] (z) is forward
		*/
		double vecPosition[3];

		/* Velocity of the pose in meters/second */
		double vecVelocity[3];

		/* Acceleration of the pose in meters/second */
		double vecAcceleration[3];

		/* Orientation of the tracker, represented as a quaternion */
		vr::HmdQuaternion_t qRotation;

		/* Angular velocity of the pose in axis-angle
		* representation. The direction is the angle of
		* rotation and the magnitude is the angle around
		* that axis in radians/second. */
		double vecAngularVelocity[3];

		/* Angular acceleration of the pose in axis-angle
		* representation. The direction is the angle of
		* rotation and the magnitude is the angle around
		* that axis in radians/second^2. */
		double vecAngularAcceleration[3];

		ETrackingResult result;

		bool poseIsValid;
		bool willDriftInYaw;
		bool shouldApplyHeadModel;
		bool deviceIsConnected;
	};
} // end namespace vr


#include <ipc_protocol.h>

namespace vrmotioncompensation
{

	class vrmotioncompensation_exception : public std::runtime_error
	{
	public:
		const int errorcode = 0;
		using std::runtime_error::runtime_error;
		vrmotioncompensation_exception(const std::string& msg, int code) : std::runtime_error(msg), errorcode(code)
		{
		}
	};

	class vrmotioncompensation_connectionerror : public vrmotioncompensation_exception
	{
		using vrmotioncompensation_exception::vrmotioncompensation_exception;
	};

	class vrmotioncompensation_invalidversion : public vrmotioncompensation_exception
	{
		using vrmotioncompensation_exception::vrmotioncompensation_exception;
	};

	class vrmotioncompensation_invalidid : public vrmotioncompensation_exception
	{
		using vrmotioncompensation_exception::vrmotioncompensation_exception;
	};

	class vrmotioncompensation_invalidtype : public vrmotioncompensation_exception
	{
		using vrmotioncompensation_exception::vrmotioncompensation_exception;
	};

	class vrmotioncompensation_notfound : public vrmotioncompensation_exception
	{
		using vrmotioncompensation_exception::vrmotioncompensation_exception;
	};

	class vrmotioncompensation_sharedmemoryerror : public vrmotioncompensation_exception
	{
		using vrmotioncompensation_exception::vrmotioncompensation_exception;
	};

	class vrmotioncompensation_alreadyinuse : public vrmotioncompensation_exception
	{
		using vrmotioncompensation_exception::vrmotioncompensation_exception;
	};

	class vrmotioncompensation_toomanydevices : public vrmotioncompensation_exception
	{
		using vrmotioncompensation_exception::vrmotioncompensation_exception;
	};


	class VRMotionCompensation
	{
	public:
		VRMotionCompensation(const std::string& driverQueue = "driver_vrmotioncompensation.server_queue", const std::string& clientQueue = "driver_vrmotioncompensation.client_queue.");
		~VRMotionCompensation();

		void connect();
		bool isConnected() const;
		void disconnect();

		void ping(bool modal = true, bool enableReply = false);

		void getDeviceInfo(uint32_t deviceId, DeviceInfo& info);;

		void setDeviceMotionCompensationMode(uint32_t MCdeviceId, uint32_t RTdeviceId, MotionCompensationMode Mode = MotionCompensationMode::Disabled, bool modal = true);

		void setMoticonCompensationSettings(double LPF_Beta, uint32_t samples, bool setZero, vr::HmdVector3d_t offsets);

		void startDebugLogger(bool enable, bool modal = true);

	private:
		std::recursive_mutex _mutex;
		uint32_t m_clientId = 0;

		bool _ipcThreadRunning = false;
		volatile bool _ipcThreadStop = false;
		std::thread _ipcThread;
		static void _ipcThreadFunc(VRMotionCompensation* _this);

		std::random_device _ipcRandomDevice;
		std::uniform_int_distribution<uint32_t> _ipcRandomDist;

		struct _ipcPromiseMapEntry
		{
			_ipcPromiseMapEntry() : isValid(false)
			{
			}
			_ipcPromiseMapEntry(std::promise<ipc::Reply>&& _promise, bool isValid = true)
				: promise(std::move(_promise)), isValid(isValid)
			{
			}
			bool isValid;
			std::promise<ipc::Reply> promise;
		};

		std::map<uint32_t, _ipcPromiseMapEntry> _ipcPromiseMap;
		std::string _ipcServerQueueName;
		std::string _ipcClientQueueName;
		boost::interprocess::message_queue* _ipcServerQueue = nullptr;
		boost::interprocess::message_queue* _ipcClientQueue = nullptr;
	};

} // end namespace vrmotioncompensation