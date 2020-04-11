#pragma once

#include <openvr_driver.h>
#include <vrmotioncompensation_types.h>
#include <openvr_math.h>
#include "../logging.h"
#include "Debugger.h"

#include <boost/timer/timer.hpp>
#include <boost/chrono/chrono.hpp>
#include <chrono\system_clocks.hpp>

// driver namespace
namespace vrmotioncompensation
{
	namespace driver
	{
		// forward declarations
		class ServerDriver;
		class DeviceManipulationHandle;


		class MotionCompensationManager
		{
		public:
			MotionCompensationManager(ServerDriver* parent) : m_parent(parent)
			{
			}
			
			void WriteDebugData();

			void InitDebugData();

			bool StartDebugData();

			void StopDebugData();

			void setMotionCompensationMode(MotionCompensationMode Mode, int MCdevice, int RTdevice);

			void setNewMotionCompensatedDevice(int MCdevice);

			void setNewReferenceTracker(int RTdevice);

			MotionCompensationMode getMotionCompensationMode()
			{
				return _motionCompensationMode;
			}

			void setLPFBeta(double NewBeta)
			{
				LPF_Beta = NewBeta;
			}

			double getLPFBeta()
			{
				return LPF_Beta;
			}

			int getMCdeviceID()
			{
				return MCdeviceID;
			}

			int getRTdeviceID()
			{
				return RTdeviceID;
			}

			bool _isMotionCompensationZeroPoseValid();
			
			void _setMotionCompensationZeroPose(const vr::DriverPose_t& pose);
			
			void _updateMotionCompensationRefPose(const vr::DriverPose_t& pose);
			
			bool _applyMotionCompensation(vr::DriverPose_t& pose, DeviceManipulationHandle* deviceInfo);

			void runFrame();

			vr::HmdVector3d_t LPF(const double RawData[3], vr::HmdVector3d_t SmoothData);

			vr::HmdVector3d_t LPF(vr::HmdVector3d_t RawData, vr::HmdVector3d_t SmoothData);

			vr::HmdQuaternion_t lowPassFilterQuaternion(vr::HmdQuaternion_t RawData, vr::HmdQuaternion_t SmoothData);			

			vr::HmdQuaternion_t Slerp(vr::HmdQuaternion_t q1, vr::HmdQuaternion_t q2, double lambda);

			vr::HmdVector3d_t ToEulerAngles(vr::HmdQuaternion_t q);

			const double AngleDifference(double angle1, double angle2);

		private:
			ServerDriver* m_parent;

			int MCdeviceID = -1;
			int RTdeviceID = -1;

			Debugger DebugLogger;

			double LPF_Beta = 0.2;

			bool _motionCompensationEnabled = false;
			MotionCompensationMode _motionCompensationMode = MotionCompensationMode::Disabled;			
			
			// Zero position
			vr::HmdVector3d_t _motionCompensationZeroPos = { 0, 0, 0 };;
			vr::HmdQuaternion_t _motionCompensationZeroRot = { 1, 0, 0, 0 };;
			bool _motionCompensationZeroPoseValid = false;
			
			// Reference position
			vr::HmdVector3d_t _motionCompensationRefPos = { 0, 0, 0 };
			vr::HmdVector3d_t _Filter_vecPosition_1 = { 0, 0, 0 };
			vr::HmdVector3d_t _Filter_vecPosition_2 = { 0, 0, 0 };
			vr::HmdVector3d_t _Filter_vecPosition_3 = { 0, 0, 0 };

			vr::HmdVector3d_t _motionCompensationRefPosVel = { 0, 0, 0 };
			vr::HmdVector3d_t _motionCompensationRefPosAcc = { 0, 0, 0 };

			vr::HmdQuaternion_t _motionCompensationRefRot = { 1, 0, 0, 0 };
			vr::HmdQuaternion_t _motionCompensationRefRotInv = { 1, 0, 0, 0 };
			vr::HmdQuaternion_t _Filter_rotPosition_1 = { 1, 0, 0, 0 };
			vr::HmdQuaternion_t _Filter_rotPosition_2 = { 1, 0, 0, 0 };
			vr::HmdQuaternion_t _Filter_rotPosition_3 = { 1, 0, 0, 0 };

			vr::HmdVector3d_t _motionCompensationRefRotVel = { 0, 0, 0 };
			vr::HmdVector3d_t _motionCompensationRefRotAcc = { 0, 0, 0 };

			bool _RefPoseValid = false;
			int _RefPoseValidCounter = 0;
		};
	}
}