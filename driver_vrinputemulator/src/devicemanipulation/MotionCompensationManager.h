#pragma once

#include <openvr_driver.h>
#include <vrinputemulator_types.h>
#include <openvr_math.h>
#include "../logging.h"


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
		timer()/* : start(Clock::now())*/
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

			double LPF(double RawData, double SmoothData);

			vr::HmdVector3d_t DebugData[10000];
			int DebugCounter = 0;
			timer<boost::chrono::high_resolution_clock> DebugTimer;
			double DebugTiming[10000];
			

		private:
			//boost::timer::auto_cpu_timer t;

			ServerDriver* m_parent;

			double LPF_Beta = 0.4;

			bool _motionCompensationEnabled = false;
			MotionCompensationMode _motionCompensationMode = MotionCompensationMode::Disabled;

			bool _motionCompensationZeroPoseValid = false;
			
			vr::HmdVector3d_t _motionCompensationZeroPos;
			vr::HmdQuaternion_t _motionCompensationZeroRot;

			bool _motionCompensationRefPoseValid = false;

			vr::HmdVector3d_t _motionCompensationRefPos;
			vr::HmdVector3d_t _mCFilter_1;
			vr::HmdVector3d_t _mCFilter_2;

			vr::HmdVector3d_t _motionCompensationRefPosVel;
			vr::HmdQuaternion_t _motionCompensationRefRot;
			vr::HmdVector3d_t _motionCompensationRefRotVel;			
		};
	}
}