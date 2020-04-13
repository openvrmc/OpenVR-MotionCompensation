#include "MotionCompensationManager.h"

#include "DeviceManipulationHandle.h"
#include "../driver/ServerDriver.h"

#include <cmath>
#include <boost/math/constants/constants.hpp>

// driver namespace
namespace vrmotioncompensation
{
	namespace driver
	{
		void MotionCompensationManager::WriteDebugData()
		{
			DebugLogger.WriteFile();
		}

		void MotionCompensationManager::InitDebugData()
		{
			DebugLogger.SetDebugNameV3("Ref Raw Pos", 0);
			DebugLogger.SetDebugNameV3("Ref Filter Pos", 1);

			DebugLogger.SetDebugNameV3("Ref Raw PosVel", 2);
			DebugLogger.SetDebugNameV3("Ref Filter PosVel", 3);

			DebugLogger.SetDebugNameV3("Ref Raw PosAcc", 4);
			DebugLogger.SetDebugNameV3("Ref Filter PosAcc", 5);

			DebugLogger.SetDebugNameV3("Ref Raw RotVel", 6);
			DebugLogger.SetDebugNameV3("Ref Filter RotVel", 7);

			DebugLogger.SetDebugNameV3("Ref Raw RotAcc", 8);
			DebugLogger.SetDebugNameV3("Ref Filter RotAcc", 9);

			DebugLogger.SetDebugNameV3("HMD Raw Pos", 10);
			DebugLogger.SetDebugNameV3("HMD MC Pos", 11);

			DebugLogger.SetDebugNameV3("HMD Raw PosVel", 12);
			DebugLogger.SetDebugNameV3("HMD MC PosVel", 13);

			DebugLogger.SetDebugNameV3("HMD Raw PosAcc", 14);
			DebugLogger.SetDebugNameV3("HMD MC PosAcc", 15);

			DebugLogger.SetDebugNameV3("HMD Raw RotVel", 16);
			DebugLogger.SetDebugNameV3("HMD MC RotVel", 17);

			DebugLogger.SetDebugNameV3("HMD Raw RotAcc", 18);
			DebugLogger.SetDebugNameV3("HMD MC RotAcc", 19);


			DebugLogger.SetDebugNameQ4("Ref Raw Rot", 0);
			DebugLogger.SetDebugNameQ4("Ref Filter Rot", 1);

			DebugLogger.SetDebugNameQ4("HMD Raw Rot", 2);
			DebugLogger.SetDebugNameQ4("HMD MC Rot", 3);

			DebugLogger.SetLPFValue(LPF_Beta);
		}

		bool MotionCompensationManager::StartDebugData()
		{
			if (_motionCompensationMode == MotionCompensationMode::ReferenceTracker && !DebugLogger.IsRunning())
			{
				InitDebugData();
				DebugLogger.Start();
				return true;
			}

			return false;
		}

		void MotionCompensationManager::StopDebugData()
		{
			DebugLogger.Stop();
			WriteDebugData();
		}

		void MotionCompensationManager::setMotionCompensationMode(MotionCompensationMode Mode, int MCdevice, int RTdevice)
		{
			MCdeviceID = MCdevice;
			RTdeviceID = RTdevice;

			if (Mode == MotionCompensationMode::ReferenceTracker)
			{
				_RefPoseValid = false;
				_RefPoseValidCounter = 0;
				_motionCompensationZeroPoseValid = false;
				_motionCompensationEnabled = true;
			}
			else
			{
				_motionCompensationEnabled = false;
			}

			_motionCompensationMode = Mode;
		}

		void MotionCompensationManager::setNewMotionCompensatedDevice(int MCdevice)
		{
			MCdeviceID = MCdevice;
		}

		void MotionCompensationManager::setNewReferenceTracker(int RTdevice)
		{
			RTdeviceID = RTdevice;
			_RefPoseValid = false;
			_motionCompensationZeroPoseValid = false;
		}

		bool MotionCompensationManager::_isMotionCompensationZeroPoseValid()
		{
			return _motionCompensationZeroPoseValid;
		}

		void MotionCompensationManager::_setMotionCompensationZeroPose(const vr::DriverPose_t& pose)
		{
			// convert pose from driver space to app space
			vr::HmdQuaternion_t tmpConj = vrmath::quaternionConjugate(pose.qWorldFromDriverRotation);

			// Save zero points
			_motionCompensationZeroPos = vrmath::quaternionRotateVector(pose.qWorldFromDriverRotation, tmpConj, pose.vecPosition, true) - pose.vecWorldFromDriverTranslation;
			_motionCompensationZeroRot = tmpConj * pose.qRotation;			

			DebugLogger.SetZeroPos(_motionCompensationZeroPos, pose.vecPosition, _motionCompensationZeroRot, pose.qRotation);

			_motionCompensationZeroPoseValid = true;
		}

		void MotionCompensationManager::_updateMotionCompensationRefPose(const vr::DriverPose_t& pose)
		{
			// From https://github.com/ValveSoftware/driver_hydra/blob/master/drivers/driver_hydra/driver_hydra.cpp Line 835:
			// "True acceleration is highly volatile, so it's not really reasonable to
			// extrapolate much from it anyway.  Passing it as 0 from any driver should
			// be fine."

			// Line 832:
			// "The tradeoff here is that setting a valid velocity causes the controllers
			// to jitter, but the controllers feel much more "alive" and lighter.
			// The jitter while stationary is more annoying than the laggy feeling caused
			// by disabling velocity (which effectively disables prediction for rendering)."
			// That means that we have to calculate the velocity to not interfere with the prediction for rendering

			// Oculus devices do use acceleration. It also seems that the HMD uses theses values for render-prediction

			// Position
			// Add a simple low pass filter
			// 1st stage
			_Filter_vecPosition_1 = LPF(pose.vecPosition, _Filter_vecPosition_1);

			// 2nd stage
			_Filter_vecPosition_2 = LPF(_Filter_vecPosition_1, _Filter_vecPosition_2);

			// 3rd stage
			_Filter_vecPosition_3 = LPF(_Filter_vecPosition_2, _Filter_vecPosition_3);

			// convert pose from driver space to app space
			vr::HmdQuaternion_t tmpConj = vrmath::quaternionConjugate(pose.qWorldFromDriverRotation);
			_motionCompensationRefPos = vrmath::quaternionRotateVector(pose.qWorldFromDriverRotation, tmpConj, _Filter_vecPosition_3, true) - pose.vecWorldFromDriverTranslation;

			// ----------------------------------------------------------------------------------------------- //
			// ----------------------------------------------------------------------------------------------- //
			// Rotation
			// Add a simple low pass filter
			// 1st stage
			_Filter_rotPosition_1 = lowPassFilterQuaternion(pose.qRotation, _Filter_rotPosition_1);

			// 2nd stage
			_Filter_rotPosition_2 = lowPassFilterQuaternion(_Filter_rotPosition_1, _Filter_rotPosition_2);

			// calculate orientation difference and its inverse
			vr::HmdQuaternion_t poseWorldRot = tmpConj * _Filter_rotPosition_2;
			_motionCompensationRefRot = poseWorldRot * vrmath::quaternionConjugate(_motionCompensationZeroRot);
			_motionCompensationRefRotInv = vrmath::quaternionConjugate(_motionCompensationRefRot);

			// ----------------------------------------------------------------------------------------------- //
			// ----------------------------------------------------------------------------------------------- //
			// Velocity and acceleration
			// Convert velocity and acceleration values into app space and undo device rotation
			vr::HmdQuaternion_t tmpRot = tmpConj * vrmath::quaternionConjugate(_Filter_rotPosition_2);
			vr::HmdQuaternion_t tmpRotInv = vrmath::quaternionConjugate(tmpRot);

			vr::HmdVector3d_t Filter_VecVelocity;

			// Calculate the difference between the smoothed and the raw position to adjust the velocity
			if (pose.vecPosition[0] != (double)0.0)
			{
				Filter_VecVelocity.v[0] = pose.vecVelocity[0] * (_Filter_vecPosition_3.v[0] / pose.vecPosition[0]);
			}
			else
			{
				Filter_VecVelocity.v[0] = pose.vecVelocity[0];
			}

			if (pose.vecPosition[1] != (double)0.0)
			{
				Filter_VecVelocity.v[1] = pose.vecVelocity[1] * (_Filter_vecPosition_3.v[1] / pose.vecPosition[1]);
			}
			else
			{
				Filter_VecVelocity.v[1] = pose.vecVelocity[1];
			}

			if (pose.vecPosition[2] != (double)0.0)
			{
				Filter_VecVelocity.v[2] = pose.vecVelocity[2] * (_Filter_vecPosition_3.v[2] / pose.vecPosition[2]);
			}
			else
			{
				Filter_VecVelocity.v[2] = pose.vecVelocity[2];
			}


			vr::HmdVector3d_t RotEulerRaw = ToEulerAngles(pose.qRotation);
			vr::HmdVector3d_t RotEulerFilter = ToEulerAngles(_Filter_rotPosition_2);
			
			vr::HmdVector3d_t Filter_vecAngularVelocity;

			if (RotEulerRaw.v[0] != (double)0.0)
			{
				Filter_vecAngularVelocity.v[0] = pose.vecAngularVelocity[0] * (1 - (AngleDifference(RotEulerRaw.v[0], RotEulerFilter.v[0]) / RotEulerRaw.v[0]));
			}
			else
			{
				Filter_vecAngularVelocity.v[0] = pose.vecAngularVelocity[0];
			}

			if (RotEulerRaw.v[1] != (double)0.0)
			{
				Filter_vecAngularVelocity.v[1] = pose.vecAngularVelocity[1] * (1 - (AngleDifference(RotEulerRaw.v[1], RotEulerFilter.v[1]) / RotEulerRaw.v[1]));
			}
			else
			{
				Filter_vecAngularVelocity.v[1] = pose.vecAngularVelocity[1];
			}

			if (RotEulerRaw.v[2] != (double)0.0)
			{
				Filter_vecAngularVelocity.v[2] = pose.vecAngularVelocity[2] * (1 - (AngleDifference(RotEulerRaw.v[2], RotEulerFilter.v[2]) / RotEulerRaw.v[2]));
			}
			else
			{
				Filter_vecAngularVelocity.v[2] = pose.vecAngularVelocity[2];
			}


			_motionCompensationRefPosVel = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, Filter_VecVelocity);			
			_motionCompensationRefRotVel = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, Filter_vecAngularVelocity);

			// Wait 10 frames before setting reference pose to valid
			if (_RefPoseValidCounter > 100)
			{
				_RefPoseValid = true;
			}
			else
			{
				_RefPoseValidCounter++;
			}			

			// ----------------------------------------------------------------------------------------------- //
			// ----------------------------------------------------------------------------------------------- //
			// Debug
			if (DebugLogger.IsRunning())
			{
				DebugLogger.CountUp();

				DebugLogger.AddDebugData(pose.vecPosition, 0);
				DebugLogger.AddDebugData(_Filter_vecPosition_3, 1);

				DebugLogger.AddDebugData(pose.vecVelocity, 2);
				DebugLogger.AddDebugData(Filter_VecVelocity, 3);

				DebugLogger.AddDebugData(pose.vecAcceleration, 4);
				DebugLogger.AddDebugData(_motionCompensationRefPosAcc, 5);

				DebugLogger.AddDebugData(pose.vecAngularVelocity, 6);
				DebugLogger.AddDebugData(Filter_vecAngularVelocity, 7);

				DebugLogger.AddDebugData(pose.vecAngularAcceleration, 8);
				DebugLogger.AddDebugData(_motionCompensationRefRotAcc, 9);

				DebugLogger.AddDebugData(pose.qRotation, 0);
				DebugLogger.AddDebugData(_Filter_rotPosition_2, 1);

				DebugLogger.SetInSync(true);
			}
			// ----------------------------------------------------------------------------------------------- //
			// ----------------------------------------------------------------------------------------------- //
			// Debug End
		}

		bool MotionCompensationManager::_applyMotionCompensation(vr::DriverPose_t& pose)
		{
			if (_motionCompensationEnabled && _motionCompensationZeroPoseValid && _RefPoseValid)
			{
				if (DebugLogger.IsRunning() && DebugLogger.IsInSync())
				{
					DebugLogger.AddDebugData(pose.vecPosition, 10);

					DebugLogger.AddDebugData(pose.vecVelocity, 12);

					DebugLogger.AddDebugData(pose.vecAcceleration, 14);

					DebugLogger.AddDebugData(pose.vecAngularVelocity, 16);

					DebugLogger.AddDebugData(pose.vecAngularAcceleration, 18);

					DebugLogger.AddDebugData(pose.qRotation, 2);
				}

				// All filter calculations are done within the function for the reference tracker, because the HMD position is updated 3x more often.
				// convert pose from driver space to app space
				vr::HmdQuaternion_t tmpConj = vrmath::quaternionConjugate(pose.qWorldFromDriverRotation);
				vr::HmdVector3d_t poseWorldPos = vrmath::quaternionRotateVector(pose.qWorldFromDriverRotation, tmpConj, pose.vecPosition, true) - pose.vecWorldFromDriverTranslation;

				// do motion compensation
				vr::HmdQuaternion_t poseWorldRot = tmpConj * pose.qRotation;
				vr::HmdVector3d_t compensatedPoseWorldPos = _motionCompensationZeroPos + vrmath::quaternionRotateVector(_motionCompensationRefRot, _motionCompensationRefRotInv, poseWorldPos - _motionCompensationRefPos, true);
				vr::HmdQuaternion_t compensatedPoseWorldRot = _motionCompensationRefRotInv * poseWorldRot;

				// Translate the motion ref vel/acc values into driver space and directly subtract them
				vr::HmdQuaternion_t tmpRot = pose.qWorldFromDriverRotation * pose.qRotation;
				vr::HmdQuaternion_t tmpRotInv = vrmath::quaternionConjugate(tmpRot);

				vr::HmdVector3d_t tmpPosVel = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, _motionCompensationRefPosVel);
				pose.vecVelocity[0] -= tmpPosVel.v[0];
				pose.vecVelocity[1] -= tmpPosVel.v[1];
				pose.vecVelocity[2] -= tmpPosVel.v[2];

				vr::HmdVector3d_t tmpRotVel = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, _motionCompensationRefRotVel);
				pose.vecAngularVelocity[0] -= tmpRotVel.v[0];
				pose.vecAngularVelocity[1] -= tmpRotVel.v[1];
				pose.vecAngularVelocity[2] -= tmpRotVel.v[2];

				// convert back to driver space
				pose.qRotation = pose.qWorldFromDriverRotation * compensatedPoseWorldRot;
				vr::HmdVector3d_t adjPoseDriverPos = vrmath::quaternionRotateVector(pose.qWorldFromDriverRotation, tmpConj, compensatedPoseWorldPos + pose.vecWorldFromDriverTranslation);
				pose.vecPosition[0] = adjPoseDriverPos.v[0];
				pose.vecPosition[1] = adjPoseDriverPos.v[1];
				pose.vecPosition[2] = adjPoseDriverPos.v[2];

				if (DebugLogger.IsRunning() && DebugLogger.IsInSync())
				{
					DebugLogger.AddDebugData(pose.vecPosition, 11);

					DebugLogger.AddDebugData(pose.vecVelocity, 13);

					DebugLogger.AddDebugData(pose.vecAcceleration, 15);

					DebugLogger.AddDebugData(pose.vecAngularVelocity, 17);

					DebugLogger.AddDebugData(pose.vecAngularAcceleration, 19);

					DebugLogger.AddDebugData(pose.qRotation, 3);

					DebugLogger.SetInSync(false);
				}
			}
			return true;
		}

		void MotionCompensationManager::runFrame()
		{

		}

		//Low Pass Filter for 3d Vectors
		vr::HmdVector3d_t MotionCompensationManager::LPF(const double RawData[3], vr::HmdVector3d_t SmoothData)
		{
			vr::HmdVector3d_t RetVal;

			RetVal.v[0] = SmoothData.v[0] - (LPF_Beta * (SmoothData.v[0] - RawData[0]));
			RetVal.v[1] = SmoothData.v[1] - (LPF_Beta * (SmoothData.v[1] - RawData[1]));
			RetVal.v[2] = SmoothData.v[2] - (LPF_Beta * (SmoothData.v[2] - RawData[2]));

			return RetVal;
		}

		//Low Pass Filter for 3d Vectors
		vr::HmdVector3d_t MotionCompensationManager::LPF(vr::HmdVector3d_t RawData, vr::HmdVector3d_t SmoothData)
		{
			vr::HmdVector3d_t RetVal;

			RetVal.v[0] = SmoothData.v[0] - (LPF_Beta * (SmoothData.v[0] - RawData.v[0]));
			RetVal.v[1] = SmoothData.v[1] - (LPF_Beta * (SmoothData.v[1] - RawData.v[1]));
			RetVal.v[2] = SmoothData.v[2] - (LPF_Beta * (SmoothData.v[2] - RawData.v[2]));

			return RetVal;
		}

		//Low Pass Filter for quaternion
		vr::HmdQuaternion_t MotionCompensationManager::lowPassFilterQuaternion(vr::HmdQuaternion_t RawData, vr::HmdQuaternion_t SmoothData)
		{
			return Slerp(SmoothData, RawData, LPF_Beta);
		}

		//Spherical Linear Interpolation for Quaternions
		vr::HmdQuaternion_t MotionCompensationManager::Slerp(vr::HmdQuaternion_t q1, vr::HmdQuaternion_t q2, double lambda)
		{
			vr::HmdQuaternion_t qr;

			double dotproduct = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
			
			// if q1 and q2 are the same, we can return either of the values
			if (dotproduct >= 1.0 || dotproduct <= -1.0)
			{
				return q1;
			}
			
			double theta, st, sut, sout, coeff1, coeff2;

			// algorithm adapted from Shoemake's paper
			lambda = lambda / 2.0;

			theta = (double)acos(dotproduct);
			if (theta < 0.0) theta = -theta;

			st = (double)sin(theta);
			sut = (double)sin(lambda * theta);
			sout = (double)sin((1 - lambda) * theta);
			coeff1 = sout / st;
			coeff2 = sut / st;

			qr.x = coeff1 * q1.x + coeff2 * q2.x;
			qr.y = coeff1 * q1.y + coeff2 * q2.y;
			qr.z = coeff1 * q1.z + coeff2 * q2.z;
			qr.w = coeff1 * q1.w + coeff2 * q2.w;


			//Normalize
			double norm = sqrt(qr.x * qr.x + qr.y * qr.y + qr.z * qr.z + qr.w * qr.w);
			qr.x /= norm;
			qr.y /= norm;
			qr.z /= norm;

			return qr;
		}

		//Convert Quaternion to Euler Angles in Radians
		vr::HmdVector3d_t MotionCompensationManager::ToEulerAngles(vr::HmdQuaternion_t q)
		{
			vr::HmdVector3d_t angles;

			// roll (x-axis rotation)
			double sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
			double cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
			angles.v[0] = std::atan2(sinr_cosp, cosr_cosp);

			// pitch (y-axis rotation)
			double sinp = 2 * (q.w * q.y - q.z * q.x);

			if (std::abs(sinp) >= 1)
			{
				angles.v[1] = std::copysign(boost::math::constants::pi<double>() / 2, sinp); // use 90 degrees if out of range
			}
			else
			{
				angles.v[1] = std::asin(sinp);
			}

			// yaw (z-axis rotation)
			double siny_cosp = 2 * (q.w * q.z + q.x * q.y);
			double cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
			angles.v[2] = std::atan2(siny_cosp, cosy_cosp);

			return angles;
		}

		//Returns the shortest difference between to angles
		const double MotionCompensationManager::AngleDifference(double Raw, double New)
		{
			double diff = fmod((New - Raw + (double)180), (double)360) - (double)180;
			return diff < -(double)180 ? diff + (double)360 : diff;
		}
	}
}