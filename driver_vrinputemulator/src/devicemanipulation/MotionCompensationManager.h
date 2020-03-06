#pragma once

#include <openvr_driver.h>
#include <vrinputemulator_types.h>
#include <openvr_math.h>
#include "../logging.h"



// driver namespace
namespace vrinputemulator
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


		private:
			ServerDriver* m_parent;

			double LPF_Beta = 0.4;

			bool _motionCompensationEnabled = false;
			MotionCompensationMode _motionCompensationMode = MotionCompensationMode::Disabled;

			bool _motionCompensationZeroPoseValid = false;
			
			vr::HmdVector3d_t _motionCompensationZeroPos;
			vr::HmdQuaternion_t _motionCompensationZeroRot;

			bool _motionCompensationRefPoseValid = false;
			vr::HmdVector3d_t _motionCompensationRefPos;
			vr::HmdVector3d_t _motionCompensationRefPosVel;
			vr::HmdQuaternion_t _motionCompensationRefRot;
			vr::HmdVector3d_t _motionCompensationRefRotVel;			
		};
	}
}