#pragma once

#include <openvr_driver.h>
#include <vrinputemulator_types.h>
#include <openvr_math.h>
#include "../logging.h"
#include "Debugger.h"

#include <boost/timer/timer.hpp>
#include <boost/chrono/chrono.hpp>
#include <chrono\system_clocks.hpp>

// driver namespace
namespace vrinputemulator
{
	template< class Clock > class timer
	{
		typename Clock::time_point tstart;
	public:
		timer()
		{
		}
		typename Clock::duration elapsed() const
		{
			return Clock::now() - tstart;
		}
		double seconds() const
		{
			return elapsed().count() * ((double)Clock::period::num / Clock::period::den);
		}
		void start()
		{
			tstart = Clock::now();
		}
	};

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

			void setMotionCompensationMode(MotionCompensationMode Mode);

			void setLPFBeta(double NewBeta)
			{
				LPF_Beta = NewBeta;
			}

			double getLPFBeta()
			{
				return LPF_Beta;
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

			/*vr::HmdVector3d_t DebugData_RawXYZ[10000];
			vr::HmdVector3d_t DebugData_FilterXYZ[10000];
			vr::HmdVector3d_t DebugData_RawRot[10000];
			vr::HmdVector3d_t DebugData_FilterRot[10000];
			int DebugCounter = 0;
			timer<boost::chrono::high_resolution_clock> DebugTimer;
			double DebugTiming[10000];*/
			

		private:
			ServerDriver* m_parent;

			Debugger DebugLogger;

			double LPF_Beta = 0.4;

			bool _motionCompensationEnabled = false;
			MotionCompensationMode _motionCompensationMode = MotionCompensationMode::Disabled;			
			
			// Zero position
			vr::HmdVector3d_t _motionCompensationZeroPos;
			vr::HmdQuaternion_t _motionCompensationZeroRot;
			bool _motionCompensationZeroPoseValid = false;
			
			// Reference position
			vr::HmdVector3d_t _motionCompensationRefPos;
			vr::HmdVector3d_t _Filter_vecPosition_1;
			vr::HmdVector3d_t _Filter_vecPosition_2;
			vr::HmdVector3d_t _Filter_vecPosition_3;

			vr::HmdVector3d_t _motionCompensationRefPosVel;
			vr::HmdVector3d_t _motionCompensationRefPosAcc;
			//vr::HmdQuaternion_t _motionCompensationRefRot;
			

			vr::HmdQuaternion_t _motionCompensationRefRot;
			vr::HmdQuaternion_t _motionCompensationRefRotInv;
			vr::HmdQuaternion_t _Filter_rotPosition_1;
			vr::HmdQuaternion_t _Filter_rotPosition_2;
			vr::HmdQuaternion_t _Filter_rotPosition_3;

			vr::HmdVector3d_t _motionCompensationRefRotVel;			
			vr::HmdVector3d_t _motionCompensationRefRotAcc;

			bool _motionCompensationRefPoseValid = false;
		};
	}
}