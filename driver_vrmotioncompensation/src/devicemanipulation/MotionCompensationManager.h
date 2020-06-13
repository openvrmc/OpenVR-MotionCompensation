#pragma once

#include <openvr_driver.h>
#include <vrmotioncompensation_types.h>
#include <openvr_math.h>
#include "../logging.h"
#include "Debugger.h"

#include <boost/timer/timer.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/interprocess/windows_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>

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

			bool setMotionCompensationMode(MotionCompensationMode Mode, int MCdevice, int RTdevice);

			void setNewMotionCompensatedDevice(int MCdevice);

			void setNewReferenceTracker(int RTdevice);

			MotionCompensationMode getMotionCompensationMode()
			{
				return _motionCompensationMode;
			}

			void setAlpha(uint32_t samples);

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

			void setZeroMode(bool setZero);

			void setOffsets(vr::HmdVector3d_t offsets)
			{
				_offset = offsets;
			}

			bool _isMotionCompensationZeroPoseValid();
			
			void _resetMotionCompensationZeroPose();

			void _setMotionCompensationZeroPose(const vr::DriverPose_t& pose);
			
			void _updateMotionCompensationRefPose(const vr::DriverPose_t& pose);
			
			bool _applyMotionCompensation(vr::DriverPose_t& pose);

			void runFrame();

			double vecVelocity(double time, const double vecPosition, const double Old_vecPosition);

			double vecAcceleration(double time, const double vecVelocity, const double Old_vecVelocity);

			double rotVelocity(double time, const double vecAngle, const double Old_vecAngle);

			double MotionCompensationManager::DEMA(const double RawData, int Axis);

			vr::HmdVector3d_t LPF(const double RawData[3], vr::HmdVector3d_t SmoothData);

			vr::HmdVector3d_t LPF(vr::HmdVector3d_t RawData, vr::HmdVector3d_t SmoothData);

			vr::HmdQuaternion_t lowPassFilterQuaternion(vr::HmdQuaternion_t RawData, vr::HmdQuaternion_t SmoothData);			

			vr::HmdQuaternion_t Slerp(vr::HmdQuaternion_t q1, vr::HmdQuaternion_t q2, double lambda);

			vr::HmdVector3d_t ToEulerAngles(vr::HmdQuaternion_t q);

			const double AngleDifference(double angle1, double angle2);

			vr::HmdVector3d_t Transform(vr::HmdVector3d_t VecRotation, vr::HmdVector3d_t VecPosition, vr::HmdVector3d_t point);

			vr::HmdVector3d_t Transform(vr::HmdQuaternion_t quat, vr::HmdVector3d_t VecPosition, vr::HmdVector3d_t point);

			vr::HmdVector3d_t Transform(vr::HmdVector3d_t VecRotation, vr::HmdVector3d_t VecPosition, vr::HmdVector3d_t centerOfRotation, vr::HmdVector3d_t point);

		private:
			ServerDriver* m_parent;

			boost::interprocess::windows_shared_memory _shdmem;

			int MCdeviceID = -1;
			int RTdeviceID = -1;
			long long _RefTrackerLastTime = -1;
			vr::DriverPose_t RefTrackerlastPose;
			vr::HmdVector3d_t RotEulerFilterOld = {0, 0, 0};

			Debugger DebugLogger;

			double LPF_Beta = 0.2;
			double _alpha = -1.0;
			uint32_t _samples = 100;
			bool _SetZeroMode = false;

			bool _motionCompensationEnabled = false;
			MotionCompensationMode _motionCompensationMode = MotionCompensationMode::Disabled;			
			
			// Offset data
			vr::HmdVector3d_t _offset = { 0, 0, 0 };

			// Zero position
			vr::HmdVector3d_t _motionCompensationZeroPos = { 0, 0, 0 };
			vr::HmdQuaternion_t _motionCompensationZeroRot = { 1, 0, 0, 0 };
			bool _motionCompensationZeroPoseValid = false;
			
			// Reference position
			vr::HmdVector3d_t _motionCompensationRefPos = { 0, 0, 0 };
			vr::HmdVector3d_t _Filter_vecPosition_1 = { 0, 0, 0 };
			vr::HmdVector3d_t _FilterOld_vecPosition_1 = { 0, 0, 0 };
			vr::HmdVector3d_t _FilterOld_vecPosition_2 = { 0, 0, 0 };

			vr::HmdVector3d_t _motionCompensationRefPosVel = { 0, 0, 0 };
			vr::HmdVector3d_t _motionCompensationRefPosAcc = { 0, 0, 0 };

			vr::HmdQuaternion_t _motionCompensationRefRot = { 1, 0, 0, 0 };
			vr::HmdQuaternion_t _motionCompensationRefRotInv = { 1, 0, 0, 0 };
			vr::HmdQuaternion_t _Filter_rotPosition_1 = { 1, 0, 0, 0 };
			vr::HmdQuaternion_t _Filter_rotPosition_2 = { 1, 0, 0, 0 };

			vr::HmdVector3d_t _motionCompensationRefRotVel = { 0, 0, 0 };
			vr::HmdVector3d_t _motionCompensationRefRotAcc = { 0, 0, 0 };

			bool _RefPoseValid = false;
			int _RefPoseValidCounter = 0;

			//vr::HmdVector3d_t _centerOfRotation = { 0, 0, 0 };
			//vr::HmdVector3d_t _translation = { 0, 0, 0 };
		};
	}
}