#include "MotionCompensationManager.h"

#include "DeviceManipulationHandle.h"
#include "../driver/ServerDriver.h"


// driver namespace
namespace vrinputemulator
{
	namespace driver
	{
		void MotionCompensationManager::setMotionCompensationMode(MotionCompensationMode Mode)
		{
			if (Mode == MotionCompensationMode::ReferenceTracker)
			{
				_motionCompensationRefPoseValid = false;
				_motionCompensationZeroPoseValid = false;
				_motionCompensationEnabled = true;
			}
			else
			{
				_motionCompensationEnabled = false;
			}

			_motionCompensationMode = Mode;
		}

		bool MotionCompensationManager::_isMotionCompensationZeroPoseValid()
		{
			return _motionCompensationZeroPoseValid;
		}

		void MotionCompensationManager::_setMotionCompensationZeroPose(const vr::DriverPose_t& pose)
		{
			_motionCompensationZeroPos.v[0] = pose.vecPosition[0];
			_motionCompensationZeroPos.v[1] = pose.vecPosition[1];
			_motionCompensationZeroPos.v[2] = pose.vecPosition[2];
			
			_motionCompensationZeroRot = pose.qRotation;

			_motionCompensationZeroPoseValid = true;

			/*LOG(DEBUG) << "DriverPose.vecPosition [0]:" << pose.vecPosition[0] << " [1]: " << pose.vecPosition[1] << " [2]: " << pose.vecPosition[2];
			LOG(DEBUG) << "DriverPose.vecWorldFromDriverTranslation [0]:" << pose.vecWorldFromDriverTranslation[0] << " [1]: " << pose.vecWorldFromDriverTranslation[1] << " [2]: " << pose.vecWorldFromDriverTranslation[2];
			LOG(DEBUG) << "pose Velocity [0]: " << pose.vecVelocity[0] << " [1]: " << pose.vecVelocity[1] << " [2]: " << pose.vecVelocity[2];
			LOG(DEBUG) << "pose Acceleration [0]: " << pose.vecAcceleration[0] << " [1]: " << pose.vecAcceleration[1] << " [2]: " << pose.vecAcceleration[2];
			//LOG(DEBUG) << "_motionCompensationZeroPos [0]:" << _motionCompensationZeroPos.v[0] << " [1]: " << _motionCompensationZeroPos.v[1] << " [2]: " << _motionCompensationZeroPos.v[2];
			
			
			LOG(DEBUG) << "DriverPose.qRotation w: " << pose.qRotation.w << " x: " << pose.qRotation.x << " y: " << pose.qRotation.y << " z: " << pose.qRotation.z;
			LOG(DEBUG) << "DriverPose.qWorldFromDriverRotation w: " << pose.qWorldFromDriverRotation.w << " x: " << pose.qWorldFromDriverRotation.x << " y: " << pose.qWorldFromDriverRotation.y << " z: " << pose.qWorldFromDriverRotation.z;
			LOG(DEBUG) << "pose Angular Velocity [0]: " << pose.vecAngularVelocity[0] << " [1]: " << pose.vecAngularVelocity[1] << " [2]: " << pose.vecAngularVelocity[2];
			LOG(DEBUG) << "pose Angular Acceleration [0]: " << pose.vecAngularAcceleration[0] << " [1]: " << pose.vecAngularAcceleration[1] << " [2]: " << pose.vecAngularAcceleration[2];*/

			/*// convert pose from driver space to app space
			vr::HmdQuaternion_t tmpConj = vrmath::quaternionConjugate(pose.qWorldFromDriverRotation);

			// Save zero points
			_motionCompensationZeroPos = vrmath::quaternionRotateVector(pose.qWorldFromDriverRotation, tmpConj, pose.vecPosition, true) - pose.vecWorldFromDriverTranslation;
			_motionCompensationZeroRot = tmpConj * pose.qRotation;

			

			_motionCompensationZeroPoseValid = true;*/
		}

		void MotionCompensationManager::_updateMotionCompensationRefPose(const vr::DriverPose_t& pose)
		{
			// From https://github.com/ValveSoftware/driver_hydra/blob/master/drivers/driver_hydra/driver_hydra.cpp Line 835:
			// "True acceleration is highly volatile, so it's not really reasonable to
			// extrapolate much from it anyway.  Passing it as 0 from any driver should
			// be fine."
			// A test showed that both acceleration values are 0. So we can ignore them for motion compensation.
			// This may change with different devices.
			
			// Line 832:
			// "The tradeoff here is that setting a valid velocity causes the controllers
			// to jitter, but the controllers feel much more "alive" and lighter.
			// The jitter while stationary is more annoying than the laggy feeling caused
			// by disabling velocity (which effectively disables prediction for rendering)."
			// That means that we have to calculate the velocity to not interfere with the prediction for rendering

			//Position
			//Add a simple low pass filter
			_motionCompensationRefPos.v[0] = LPF(pose.vecPosition[0], _motionCompensationRefPos.v[0]);
			_motionCompensationRefPos.v[1] = LPF(pose.vecPosition[1], _motionCompensationRefPos.v[1]);
			_motionCompensationRefPos.v[2] = LPF(pose.vecPosition[2], _motionCompensationRefPos.v[2]);

			//Now we need to calculate the difference between the smoothed and the raw position to adjust the velocity
			if (pose.vecPosition[0] != (double)0.0)
			{
				_motionCompensationRefPosVel.v[0] = pose.vecVelocity[0] * (_motionCompensationRefPos.v[0] / pose.vecPosition[0]);
			}
			else
			{
				_motionCompensationRefPosVel.v[0] = pose.vecVelocity[0];
			}
			
			if (pose.vecPosition[1] != (double)0.0)
			{
				_motionCompensationRefPosVel.v[1] = pose.vecVelocity[1] * (_motionCompensationRefPos.v[1] / pose.vecPosition[1]);
			}
			else
			{
				_motionCompensationRefPosVel.v[1] = pose.vecVelocity[1];
			}
			

			if (pose.vecPosition[2] != (double)0.0)
			{
				_motionCompensationRefPosVel.v[2] = pose.vecVelocity[2] * (_motionCompensationRefPos.v[2] / pose.vecPosition[2]);
			}
			else
			{
				_motionCompensationRefPosVel.v[2] = pose.vecVelocity[2];
			}
			


			//Rotation ToDo
			_motionCompensationRefRot = pose.qRotation;

			_motionCompensationRefRotVel.v[0] = pose.vecAngularVelocity[0];
			_motionCompensationRefRotVel.v[1] = pose.vecAngularVelocity[1];
			_motionCompensationRefRotVel.v[2] = pose.vecAngularVelocity[2];			

			//Position is valid (only needed for the first frame)
			_motionCompensationRefPoseValid = true;


			/*// convert pose from driver space to app space
			vr::HmdQuaternion_t tmpConj = vrmath::quaternionConjugate(pose.qWorldFromDriverRotation);
			_motionCompensationRefPos = vrmath::quaternionRotateVector(pose.qWorldFromDriverRotation, tmpConj, pose.vecPosition, true) - pose.vecWorldFromDriverTranslation;
			vr::HmdQuaternion_t poseWorldRot = tmpConj * pose.qRotation;

			// calculate orientation difference and its inverse
			_motionCompensationRotDiff = poseWorldRot * vrmath::quaternionConjugate(_motionCompensationZeroRot);
			_motionCompensationRotDiffInv = vrmath::quaternionConjugate(_motionCompensationRotDiff);

			// Convert velocity and acceleration values into app space and undo device rotation
			vr::HmdQuaternion_t tmpRot = tmpConj * vrmath::quaternionConjugate(pose.qRotation);
			vr::HmdQuaternion_t tmpRotInv = vrmath::quaternionConjugate(tmpRot);

			_motionCompensationRefPosVel = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, { pose.vecVelocity[0], pose.vecVelocity[1], pose.vecVelocity[2] });
			_motionCompensationRefPosAcc = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, { pose.vecAcceleration[0], pose.vecAcceleration[1], pose.vecAcceleration[2] });
			_motionCompensationRefRotVel = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, { pose.vecAngularVelocity[0], pose.vecAngularVelocity[1], pose.vecAngularVelocity[2] });
			_motionCompensationRefRotAcc = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, { pose.vecAngularAcceleration[0], pose.vecAngularAcceleration[1], pose.vecAngularAcceleration[2] });
			_motionCompensationRefPoseValid = true;*/
		}

		bool MotionCompensationManager::_applyMotionCompensation(vr::DriverPose_t& pose, DeviceManipulationHandle* deviceInfo)
		{
			if (_motionCompensationEnabled && _motionCompensationZeroPoseValid && _motionCompensationRefPoseValid)
			{
				//All filter calculations are done within the function for the reference tracker, because the HMD position is updated 3x more often.
				//Position compensation
				pose.vecVelocity[0] -= _motionCompensationRefPosVel.v[0];
				pose.vecVelocity[1] -= _motionCompensationRefPosVel.v[1];
				pose.vecVelocity[2] -= _motionCompensationRefPosVel.v[2];
				
				pose.vecPosition[0] += _motionCompensationZeroPos.v[0] - _motionCompensationRefPos.v[0];
				pose.vecPosition[1] += _motionCompensationZeroPos.v[1] - _motionCompensationRefPos.v[1];
				pose.vecPosition[2] += _motionCompensationZeroPos.v[2] - _motionCompensationRefPos.v[2];

				//pose.vecAngularVelocity[0] -= _motionCompensationRefRotVel.v[0];
				//pose.vecAngularVelocity[1] -= _motionCompensationRefRotVel.v[1];
				//pose.vecAngularVelocity[2] -= _motionCompensationRefRotVel.v[2];

				//pose.qRotation.w -= _motionCompensationRefRot.w - _motionCompensationZeroRot.w;
				//pose.qRotation.x -= _motionCompensationRefRot.x - _motionCompensationZeroRot.x;
				//pose.qRotation.y -= _motionCompensationRefRot.y - _motionCompensationZeroRot.y;
				//pose.qRotation.z -= _motionCompensationRefRot.z - _motionCompensationZeroRot.z;

				/*pose.vecPosition[0] = adjPoseDriverPos.v[0];
				pose.vecPosition[1] = adjPoseDriverPos.v[1];
				pose.vecPosition[2] = adjPoseDriverPos.v[2];*/

				/*// convert pose from driver space to app space
				vr::HmdQuaternion_t tmpConj = vrmath::quaternionConjugate(pose.qWorldFromDriverRotation);
				vr::HmdVector3d_t poseWorldPos = vrmath::quaternionRotateVector(pose.qWorldFromDriverRotation, tmpConj, pose.vecPosition, true) - pose.vecWorldFromDriverTranslation;
				vr::HmdQuaternion_t poseWorldRot = tmpConj * pose.qRotation;

				// do motion compensation
				vr::HmdVector3d_t compensatedPoseWorldPos = _motionCompensationZeroPos + vrmath::quaternionRotateVector(_motionCompensationRotDiff, _motionCompensationRotDiffInv, poseWorldPos - _motionCompensationRefPos, true);
				vr::HmdQuaternion_t compensatedPoseWorldRot = _motionCompensationRotDiffInv * poseWorldRot;

				auto now = std::chrono::duration_cast <std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
				
				// We translate the motion ref vel/acc values into driver space and directly substract them
				if (_motionCompensationRefPoseValid)
				{
					vr::HmdQuaternion_t tmpRot = pose.qWorldFromDriverRotation * pose.qRotation;
					vr::HmdQuaternion_t tmpRotInv = vrmath::quaternionConjugate(tmpRot);

					vr::HmdVector3d_t tmpPosVel = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, _motionCompensationRefPosVel);
					pose.vecVelocity[0] -= tmpPosVel.v[0];
					pose.vecVelocity[1] -= tmpPosVel.v[1];
					pose.vecVelocity[2] -= tmpPosVel.v[2];

					vr::HmdVector3d_t tmpPosAcc = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, _motionCompensationRefPosAcc);
					pose.vecAcceleration[0] -= tmpPosAcc.v[0];
					pose.vecAcceleration[1] -= tmpPosAcc.v[1];
					pose.vecAcceleration[2] -= tmpPosAcc.v[2];

					vr::HmdVector3d_t tmpRotVel = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, _motionCompensationRefRotVel);
					pose.vecAngularVelocity[0] -= tmpRotVel.v[0];
					pose.vecAngularVelocity[1] -= tmpRotVel.v[1];
					pose.vecAngularVelocity[2] -= tmpRotVel.v[2];

					vr::HmdVector3d_t tmpRotAcc = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, _motionCompensationRefRotAcc);
					pose.vecAngularAcceleration[0] -= tmpRotAcc.v[0];
					pose.vecAngularAcceleration[1] -= tmpRotAcc.v[1];
					pose.vecAngularAcceleration[2] -= tmpRotAcc.v[2];
				}

				// convert back to driver space
				pose.qRotation = pose.qWorldFromDriverRotation * compensatedPoseWorldRot;
				vr::HmdVector3d_t adjPoseDriverPos = vrmath::quaternionRotateVector(pose.qWorldFromDriverRotation, tmpConj, compensatedPoseWorldPos + pose.vecWorldFromDriverTranslation);
				pose.vecPosition[0] = adjPoseDriverPos.v[0];
				pose.vecPosition[1] = adjPoseDriverPos.v[1];
				pose.vecPosition[2] = adjPoseDriverPos.v[2];*/
			}
			return true;
		}

		void MotionCompensationManager::runFrame()
		{

		}

		double MotionCompensationManager::LPF(double RawData, double SmoothData)
		{
			return SmoothData - (LPF_Beta * (SmoothData - RawData));
		}
	}
}