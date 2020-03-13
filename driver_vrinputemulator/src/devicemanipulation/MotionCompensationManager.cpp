#include "MotionCompensationManager.h"

#include "DeviceManipulationHandle.h"
#include "../driver/ServerDriver.h"

#include <boost/qvm/quat_operations.hpp>
#include <boost/math/quaternion.hpp>

// driver namespace
namespace vrmotioncompensation
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
			DebugTimer.start();
		}

		bool MotionCompensationManager::_isMotionCompensationZeroPoseValid()
		{
			return _motionCompensationZeroPoseValid;
		}

		void MotionCompensationManager::_setMotionCompensationZeroPose(const vr::DriverPose_t& pose)
		{
			/*_motionCompensationZeroPos.v[0] = pose.vecPosition[0];
			_motionCompensationZeroPos.v[1] = pose.vecPosition[1];
			_motionCompensationZeroPos.v[2] = pose.vecPosition[2];
			
			_motionCompensationZeroRot = pose.qRotation;

			_motionCompensationZeroPoseValid = true;*/

			// convert pose from driver space to app space
			vr::HmdQuaternion_t tmpConj = vrmath::quaternionConjugate(pose.qWorldFromDriverRotation);

			// Save zero points
			_motionCompensationZeroPos = vrmath::quaternionRotateVector(pose.qWorldFromDriverRotation, tmpConj, pose.vecPosition, true) - pose.vecWorldFromDriverTranslation;
			_motionCompensationZeroRot = tmpConj * pose.qRotation;			

			_motionCompensationZeroPoseValid = true;
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


			// Rotation
			// Add a simple low pass filter
			// 1st stage
			_Filter_rotPosition_1 = lowPassFilterQuaternion(pose.qRotation, _Filter_rotPosition_1);

			// 2nd stage
			_Filter_rotPosition_2 = lowPassFilterQuaternion(_Filter_rotPosition_1, _Filter_rotPosition_2);

			// 3rd stage
			_Filter_rotPosition_3 = lowPassFilterQuaternion(_Filter_rotPosition_2, _Filter_rotPosition_3);

			// calculate orientation difference and its inverse
			vr::HmdQuaternion_t poseWorldRot = tmpConj * _Filter_rotPosition_3;
			_motionCompensationRefRot = poseWorldRot * vrmath::quaternionConjugate(_motionCompensationZeroRot);
			_motionCompensationRefRotInv = vrmath::quaternionConjugate(_motionCompensationRefRot);




			// Convert velocity and acceleration values into app space and undo device rotation
			vr::HmdQuaternion_t tmpRot = tmpConj * vrmath::quaternionConjugate(_Filter_rotPosition_3);
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

			_motionCompensationRefPosVel = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, Filter_VecVelocity);
			_motionCompensationRefPosAcc = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, { pose.vecAcceleration[0], pose.vecAcceleration[1], pose.vecAcceleration[2] });
			_motionCompensationRefRotVel = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, { pose.vecAngularVelocity[0], pose.vecAngularVelocity[1], pose.vecAngularVelocity[2] });
			_motionCompensationRefRotAcc = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, { pose.vecAngularAcceleration[0], pose.vecAngularAcceleration[1], pose.vecAngularAcceleration[2] });
			_motionCompensationRefPoseValid = true;


			if (DebugCounter >= 10000)
			{
				DebugCounter = 0;
			}

			DebugTiming[DebugCounter] = DebugTimer.seconds();

			DebugData_RawXYZ[DebugCounter].v[0] = pose.vecPosition[0];
			DebugData_RawXYZ[DebugCounter].v[1] = pose.vecPosition[1];
			DebugData_RawXYZ[DebugCounter].v[2] = pose.vecPosition[2];

			DebugData_FilterXYZ[DebugCounter].v[0] = _Filter_vecPosition_3.v[0];
			DebugData_FilterXYZ[DebugCounter].v[1] = _Filter_vecPosition_3.v[1];
			DebugData_FilterXYZ[DebugCounter].v[2] = _Filter_vecPosition_3.v[2];

			DebugData_RawRot[DebugCounter].v[0] = pose.qRotation.w;
			DebugData_RawRot[DebugCounter].v[1] = pose.qRotation.x;
			DebugData_RawRot[DebugCounter].v[2] = pose.qRotation.y;
			DebugData_RawRot[DebugCounter].v[2] = pose.qRotation.z;

			DebugData_FilterRot[DebugCounter].v[0] = _Filter_rotPosition_3.w;
			DebugData_FilterRot[DebugCounter].v[1] = _Filter_rotPosition_3.x;
			DebugData_FilterRot[DebugCounter].v[2] = _Filter_rotPosition_3.y;
			DebugData_FilterRot[DebugCounter].v[2] = _Filter_rotPosition_3.z;

			DebugCounter++;

			// Now we need to calculate the difference between the smoothed and the raw position to adjust the velocity
			/*if (pose.vecPosition[0] != (double)0.0)
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
			}*/
			
			// Save debug data
			/*if (DebugCounter >= 10000)
			{
				DebugCounter = 0;
			}

			DebugTiming[DebugCounter] = DebugTimer.seconds();

			DebugData_RawXYZ[DebugCounter].v[0] = pose.vecPosition[0];
			DebugData_RawXYZ[DebugCounter].v[1] = pose.vecPosition[1];
			DebugData_RawXYZ[DebugCounter].v[2] = pose.vecPosition[2];

			DebugData_FilterXYZ[DebugCounter].v[0] = _motionCompensationRefPos.v[0];
			DebugData_FilterXYZ[DebugCounter].v[1] = _motionCompensationRefPos.v[1];
			DebugData_FilterXYZ[DebugCounter].v[2] = _motionCompensationRefPos.v[2];

			DebugData_RawRot[DebugCounter].v[0] = pose.vecVelocity[0];
			DebugData_RawRot[DebugCounter].v[1] = pose.vecVelocity[1];
			DebugData_RawRot[DebugCounter].v[2] = pose.vecVelocity[2];

			DebugData_FilterRot[DebugCounter].v[0] = _motionCompensationRefPosVel.v[0];
			DebugData_FilterRot[DebugCounter].v[1] = _motionCompensationRefPosVel.v[1];
			DebugData_FilterRot[DebugCounter].v[2] = _motionCompensationRefPosVel.v[2];

			DebugCounter++;*/


			//Rotation ToDo
			/*_motionCompensationRefRot = pose.qRotation;

			_motionCompensationRefRotVel.v[0] = pose.vecAngularVelocity[0];
			_motionCompensationRefRotVel.v[1] = pose.vecAngularVelocity[1];
			_motionCompensationRefRotVel.v[2] = pose.vecAngularVelocity[2];*/	

			//Position is valid (only needed for the first frame)
			_motionCompensationRefPoseValid = true;			
		}

		bool MotionCompensationManager::_applyMotionCompensation(vr::DriverPose_t& pose, DeviceManipulationHandle* deviceInfo)
		{
			if (_motionCompensationEnabled && _motionCompensationZeroPoseValid && _motionCompensationRefPoseValid)
			{
				//All filter calculations are done within the function for the reference tracker, because the HMD position is updated 3x more often.
				//Position compensation
				/*pose.vecVelocity[0] -= _motionCompensationRefPosVel.v[0];
				pose.vecVelocity[1] -= _motionCompensationRefPosVel.v[1];
				pose.vecVelocity[2] -= _motionCompensationRefPosVel.v[2];
				
				pose.vecPosition[0] += _motionCompensationZeroPos.v[0] - _motionCompensationRefPos.v[0];
				pose.vecPosition[1] += _motionCompensationZeroPos.v[1] - _motionCompensationRefPos.v[1];
				pose.vecPosition[2] += _motionCompensationZeroPos.v[2] - _motionCompensationRefPos.v[2];*/

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

				// convert pose from driver space to app space
				vr::HmdQuaternion_t tmpConj = vrmath::quaternionConjugate(pose.qWorldFromDriverRotation);
				vr::HmdVector3d_t poseWorldPos = vrmath::quaternionRotateVector(pose.qWorldFromDriverRotation, tmpConj, pose.vecPosition, true) - pose.vecWorldFromDriverTranslation;				

				// do motion compensation
				vr::HmdQuaternion_t poseWorldRot = tmpConj * pose.qRotation;
				vr::HmdVector3d_t compensatedPoseWorldPos = _motionCompensationZeroPos + vrmath::quaternionRotateVector(_motionCompensationRefRot, _motionCompensationRefRotInv, poseWorldPos - _motionCompensationRefPos, true);
				vr::HmdQuaternion_t compensatedPoseWorldRot = _motionCompensationRefRotInv * poseWorldRot;

				//auto now = std::chrono::duration_cast <std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
				
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
				pose.vecPosition[2] = adjPoseDriverPos.v[2];
			}
			return true;
		}

		void MotionCompensationManager::runFrame()
		{

		}

		vr::HmdVector3d_t MotionCompensationManager::LPF(const double RawData[3], vr::HmdVector3d_t SmoothData)
		{
			vr::HmdVector3d_t RetVal;

			RetVal.v[0] = SmoothData.v[0] - (LPF_Beta * (SmoothData.v[0] - RawData[0]));
			RetVal.v[1] = SmoothData.v[1] - (LPF_Beta * (SmoothData.v[1] - RawData[1]));
			RetVal.v[2] = SmoothData.v[2] - (LPF_Beta * (SmoothData.v[2] - RawData[2]));

			//return SmoothData - (LPF_Beta * (SmoothData - RawData));

			return RetVal;
		}

		vr::HmdVector3d_t MotionCompensationManager::LPF(vr::HmdVector3d_t RawData, vr::HmdVector3d_t SmoothData)
		{
			vr::HmdVector3d_t RetVal;

			RetVal.v[0] = SmoothData.v[0] - (LPF_Beta * (SmoothData.v[0] - RawData.v[0]));
			RetVal.v[1] = SmoothData.v[1] - (LPF_Beta * (SmoothData.v[1] - RawData.v[1]));
			RetVal.v[2] = SmoothData.v[2] - (LPF_Beta * (SmoothData.v[2] - RawData.v[2]));

			return RetVal;
		}
		
		//Function:
		vr::HmdQuaternion_t MotionCompensationManager::lowPassFilterQuaternion(vr::HmdQuaternion_t RawData, vr::HmdQuaternion_t SmoothData)
		{
			//boost::qvm::slerp(_Filter_rotPosition_1, pose.qRotation, LPF_Beta);
			/*boost::math::quaternion<double> boost_SmoothData(SmoothData.w, SmoothData.x, SmoothData.y, SmoothData.z);
			boost::math::quaternion<double> boost_RawData(RawData.w, RawData.x, RawData.y, RawData.z);			

			//boost_SmoothData = 
			boost::qvm::slerp<boost::math::quaternion<double>, boost::math::quaternion<double>, double>(boost_SmoothData, boost_RawData, LPF_Beta);

			return intermediateValue;*/

			return Slerp(SmoothData, RawData, LPF_Beta);
		}

		vr::HmdQuaternion_t MotionCompensationManager::Slerp(vr::HmdQuaternion_t q1, vr::HmdQuaternion_t q2, double lambda)
		{
			vr::HmdQuaternion_t qr;

			double dotproduct = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
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

		/*inline double MotionCompensationManager::Norm(vr::HmdQuaternion_t q1)
		{
			return sqrt(q1.x * q1.x + q1.y * q1.y + q1.z * q1.z + q1.w * q1.w);
		}

		inline void MotionCompensationManager::Normalize(vr::HmdQuaternion_t& q1)
		{
			double norm = Norm(q1);
			q1.x /= norm; q1.y /= norm; q1.z /= norm;
		}*/
	}
}