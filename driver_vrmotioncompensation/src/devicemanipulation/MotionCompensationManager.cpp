#include "MotionCompensationManager.h"

#include "DeviceManipulationHandle.h"
#include "../driver/ServerDriver.h"

#include <cmath>
#include <boost/math/constants/constants.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

// driver namespace
namespace vrmotioncompensation
{
	namespace driver
	{
		MotionCompensationManager::MotionCompensationManager(ServerDriver* parent) : m_parent(parent)
		{
			try
			{
				// create shared memory
				_shdmem = { boost::interprocess::open_or_create, "OVRMC_MMF", boost::interprocess::read_write, 32 };
				_region = { _shdmem, boost::interprocess::read_write };

				// get pointer address and fill it with data
				_poffset = static_cast<vr::HmdVector3d_t*>(_region.get_address());
				*_poffset = _offset;
			}
			catch (boost::interprocess::interprocess_exception & e)
			{				
				LOG(ERROR) << "Could not create or open shared memory. Error code " << e.get_error_code();
			}
		}

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

		bool MotionCompensationManager::setMotionCompensationMode(MotionCompensationMode Mode, int MCdevice, int RTdevice)
		{
			if (Mode == MotionCompensationMode::ReferenceTracker)
			{
				_RefPoseValid = false;
				_RefPoseValidCounter = 0;
				_motionCompensationZeroPoseValid = false;
				_motionCompensationEnabled = true;
				
				setAlpha(_samples);
			}
			else
			{
				_motionCompensationEnabled = false;
			}

			MCdeviceID = MCdevice;
			RTdeviceID = RTdevice;
			_motionCompensationMode = Mode;

			return true;
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

		void MotionCompensationManager::setAlpha(uint32_t samples)
		{
			_samples = samples;
			_alpha = 2.0 / (1.0 + (double)samples);
		}

		void MotionCompensationManager::setZeroMode(bool setZero)
		{
			_SetZeroMode = setZero;

			_motionCompensationRefPosVel = { 0, 0, 0 };
			_motionCompensationRefRotVel = { 0, 0, 0 };

			_motionCompensationRefPosAcc = { 0, 0, 0 };
			_motionCompensationRefRotAcc = { 0, 0, 0 };
		}

		void MotionCompensationManager::setOffsets(vr::HmdVector3d_t offsets)
		{
			_offset = offsets;
			*_poffset = _offset;
		}

		bool MotionCompensationManager::_isMotionCompensationZeroPoseValid()
		{
			return _motionCompensationZeroPoseValid;
		}

		void MotionCompensationManager::_resetMotionCompensationZeroPose()
		{
			_motionCompensationZeroPoseValid = false;
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
			// "The trade-off here is that setting a valid velocity causes the controllers
			// to jitter, but the controllers feel much more "alive" and lighter.
			// The jitter while stationary is more annoying than the laggy feeling caused
			// by disabling velocity (which effectively disables prediction for rendering)."
			// That means that we have to calculate the velocity to not interfere with the prediction for rendering

			// Oculus devices do use acceleration. It also seems that the HMD uses theses values for render-prediction

			vr::HmdVector3d_t Filter_VecVelocity = { 0, 0, 0 };
			vr::HmdVector3d_t Filter_VecAcceleration = { 0, 0, 0 };
			vr::HmdVector3d_t Filter_vecAngularVelocity = { 0, 0, 0 };
			vr::HmdVector3d_t Filter_vecAngularAcceleration = { 0, 0, 0 };
			vr::HmdVector3d_t RotEulerFilter = { 0, 0, 0 };

			// Get current time in microseconds and convert it to seconds
			long long now = std::chrono::duration_cast <std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			double tdiff = (double)(now - _RefTrackerLastTime) / 1.0E6 + (pose.poseTimeOffset - RefTrackerlastPose.poseTimeOffset);

			// Position
			// Add a exponential median average filter
			if (_samples >= 2)
			{
				// ----------------------------------------------------------------------------------------------- //
				// ----------------------------------------------------------------------------------------------- //
				// Position
				_Filter_vecPosition_1.v[0] = DEMA(pose.vecPosition[0], 0);
				_Filter_vecPosition_1.v[1] = DEMA(pose.vecPosition[1], 1);
				_Filter_vecPosition_1.v[2] = DEMA(pose.vecPosition[2], 2);

				// ----------------------------------------------------------------------------------------------- //
				// ----------------------------------------------------------------------------------------------- //
				// Velocity and acceleration
				if (!_SetZeroMode)
				{
					Filter_VecVelocity.v[0] = vecVelocity(tdiff, _Filter_vecPosition_1.v[0], RefTrackerlastPose.vecPosition[0]);
					Filter_VecVelocity.v[1] = vecVelocity(tdiff, _Filter_vecPosition_1.v[1], RefTrackerlastPose.vecPosition[1]);
					Filter_VecVelocity.v[2] = vecVelocity(tdiff, _Filter_vecPosition_1.v[2], RefTrackerlastPose.vecPosition[2]);

					Filter_VecAcceleration.v[0] = vecAcceleration(tdiff, Filter_VecVelocity.v[0], RefTrackerlastPose.vecVelocity[0]);
					Filter_VecAcceleration.v[1] = vecAcceleration(tdiff, Filter_VecVelocity.v[1], RefTrackerlastPose.vecVelocity[1]);
					Filter_VecAcceleration.v[2] = vecAcceleration(tdiff, Filter_VecVelocity.v[2], RefTrackerlastPose.vecVelocity[2]);
				}
			}
			else
			{
				_Filter_vecPosition_1.v[0] = pose.vecPosition[0];
				_Filter_vecPosition_1.v[1] = pose.vecPosition[1];
				_Filter_vecPosition_1.v[2] = pose.vecPosition[2];

				Filter_VecVelocity.v[0] = pose.vecVelocity[0];
				Filter_VecVelocity.v[1] = pose.vecVelocity[1];
				Filter_VecVelocity.v[2] = pose.vecVelocity[2];
			}

			// convert pose from driver space to app space
			vr::HmdQuaternion_t tmpConj = vrmath::quaternionConjugate(pose.qWorldFromDriverRotation);
			_motionCompensationRefPos = vrmath::quaternionRotateVector(pose.qWorldFromDriverRotation, tmpConj, _Filter_vecPosition_1, true) - pose.vecWorldFromDriverTranslation;

			// ----------------------------------------------------------------------------------------------- //
			// ----------------------------------------------------------------------------------------------- //
			// Rotation
			if (LPF_Beta <= 0.9999)
			{
				// 1st stage
				_Filter_rotPosition_1 = lowPassFilterQuaternion(pose.qRotation, _Filter_rotPosition_1);

				// 2nd stage
				_Filter_rotPosition_2 = lowPassFilterQuaternion(_Filter_rotPosition_1, _Filter_rotPosition_2);

				
				vr::HmdVector3d_t RotEulerFilter = ToEulerAngles(_Filter_rotPosition_2);

				if (!_SetZeroMode)
				{
					Filter_vecAngularVelocity.v[0] = rotVelocity(tdiff, RotEulerFilter.v[0], RotEulerFilterOld.v[0]);
					Filter_vecAngularVelocity.v[1] = rotVelocity(tdiff, RotEulerFilter.v[1], RotEulerFilterOld.v[1]);
					Filter_vecAngularVelocity.v[2] = rotVelocity(tdiff, RotEulerFilter.v[2], RotEulerFilterOld.v[2]);

					Filter_vecAngularAcceleration.v[0] = vecAcceleration(tdiff, Filter_vecAngularVelocity.v[0], RefTrackerlastPose.vecAngularVelocity[0]);
					Filter_vecAngularAcceleration.v[1] = vecAcceleration(tdiff, Filter_vecAngularVelocity.v[1], RefTrackerlastPose.vecAngularVelocity[1]);
					Filter_vecAngularAcceleration.v[2] = vecAcceleration(tdiff, Filter_vecAngularVelocity.v[2], RefTrackerlastPose.vecAngularVelocity[2]);
				}
			}
			else
			{
				_Filter_rotPosition_2 = pose.qRotation;

				Filter_vecAngularVelocity.v[0] = pose.vecAngularVelocity[0];
				Filter_vecAngularVelocity.v[1] = pose.vecAngularVelocity[1];
				Filter_vecAngularVelocity.v[2] = pose.vecAngularVelocity[2];

				Filter_vecAngularAcceleration.v[0] = pose.vecAngularAcceleration[0];
				Filter_vecAngularAcceleration.v[1] = pose.vecAngularAcceleration[1];
				Filter_vecAngularAcceleration.v[2] = pose.vecAngularAcceleration[2];
			}

			// calculate orientation difference and its inverse
			vr::HmdQuaternion_t poseWorldRot = tmpConj * _Filter_rotPosition_2;
			_motionCompensationRefRot = poseWorldRot * vrmath::quaternionConjugate(_motionCompensationZeroRot);
			_motionCompensationRefRotInv = vrmath::quaternionConjugate(_motionCompensationRefRot);			

			if (!_SetZeroMode)
			{
				// Convert velocity and acceleration values into app space and undo device rotation
				vr::HmdQuaternion_t tmpRot = tmpConj * vrmath::quaternionConjugate(_Filter_rotPosition_2);
				vr::HmdQuaternion_t tmpRotInv = vrmath::quaternionConjugate(tmpRot);

				_motionCompensationRefPosVel = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, Filter_VecVelocity);
				_motionCompensationRefRotVel = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, Filter_vecAngularVelocity);

				_motionCompensationRefPosAcc = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, Filter_VecAcceleration);
				_motionCompensationRefRotAcc = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, Filter_vecAngularAcceleration);
			}

			// ----------------------------------------------------------------------------------------------- //
			// ----------------------------------------------------------------------------------------------- //
			// Wait 100 frames before setting reference pose to valid
			if (_RefPoseValidCounter > 100)
			{
				_RefPoseValid = true;
			}
			else
			{
				_RefPoseValidCounter++;
			}			

			// Save last rotation and pose
			RotEulerFilterOld = RotEulerFilter;
			RefTrackerlastPose = pose;

			// ----------------------------------------------------------------------------------------------- //
			// ----------------------------------------------------------------------------------------------- //
			// Debug
			if (DebugLogger.IsRunning())
			{
				DebugLogger.CountUp();

				DebugLogger.AddDebugData(pose.vecPosition, 0);
				DebugLogger.AddDebugData(_Filter_vecPosition_1, 1);

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
				// Convert pose from driver space to app space
				vr::HmdQuaternion_t tmpConj = vrmath::quaternionConjugate(pose.qWorldFromDriverRotation);
				vr::HmdVector3d_t poseWorldPos = vrmath::quaternionRotateVector(pose.qWorldFromDriverRotation, tmpConj, pose.vecPosition, true) - pose.vecWorldFromDriverTranslation;

				// Do motion compensation
				vr::HmdQuaternion_t poseWorldRot = tmpConj * pose.qRotation;
				vr::HmdVector3d_t compensatedPoseWorldPos = _motionCompensationZeroPos + vrmath::quaternionRotateVector(_motionCompensationRefRot, _motionCompensationRefRotInv, poseWorldPos - _motionCompensationRefPos, true);
				vr::HmdQuaternion_t compensatedPoseWorldRot = _motionCompensationRefRotInv * poseWorldRot;

				// Translate the motion ref Velocity / Acceleration values into driver space and directly subtract them
				vr::HmdQuaternion_t tmpRot = pose.qWorldFromDriverRotation * pose.qRotation;
				vr::HmdQuaternion_t tmpRotInv = vrmath::quaternionConjugate(tmpRot);

				if (_SetZeroMode)
				{
					pose.vecVelocity[0] = 0.0;
					pose.vecVelocity[1] = 0.0;
					pose.vecVelocity[2] = 0.0;

					pose.vecAngularVelocity[0] = 0.0;
					pose.vecAngularVelocity[1] = 0.0;
					pose.vecAngularVelocity[2] = 0.0;

					pose.vecAcceleration[0] = 0.0;
					pose.vecAcceleration[1] = 0.0;
					pose.vecAcceleration[2] = 0.0;

					pose.vecAngularAcceleration[0] = 0.0;
					pose.vecAngularAcceleration[1] = 0.0;
					pose.vecAngularAcceleration[2] = 0.0;
				}
				else
				{
					vr::HmdVector3d_t tmpPosVel = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, _motionCompensationRefPosVel);
					pose.vecVelocity[0] -= tmpPosVel.v[0];
					pose.vecVelocity[1] -= tmpPosVel.v[1];
					pose.vecVelocity[2] -= tmpPosVel.v[2];

					vr::HmdVector3d_t tmpRotVel = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, _motionCompensationRefRotVel);
					pose.vecAngularVelocity[0] -= tmpRotVel.v[0];
					pose.vecAngularVelocity[1] -= tmpRotVel.v[1];
					pose.vecAngularVelocity[2] -= tmpRotVel.v[2];

					vr::HmdVector3d_t tmpPosAcc = vrmath::quaternionRotateVector(tmpRot, tmpRotInv, _motionCompensationRefPosAcc);
					pose.vecAcceleration[0] -= tmpPosAcc.v[0];
					pose.vecAcceleration[1] -= tmpPosAcc.v[1];
					pose.vecAcceleration[2] -= tmpPosAcc.v[2];

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

		double MotionCompensationManager::vecVelocity(double time, const double vecPosition, const double Old_vecPosition)
		{
			double NewVelocity = 0.0;
			
			if (time != (double)0.0)
			{
				NewVelocity = (vecPosition - Old_vecPosition) / time;
			}

			return NewVelocity;
		}

		double MotionCompensationManager::vecAcceleration(double time, const double vecVelocity, const double Old_vecVelocity)
		{
			double NewAcceleration = 0.0;

			if (time != (double)0.0)
			{
				NewAcceleration = (vecVelocity - vecVelocity) / time;
			}

			return NewAcceleration;
		}

		double MotionCompensationManager::rotVelocity(double time, const double vecAngle, const double Old_vecAngle)
		{
			double NewVelocity = 0.0;

			if (time != (double)0.0)
			{
				NewVelocity = (1 - AngleDifference(vecAngle, Old_vecAngle)) / time;
			}

			return NewVelocity;
		}

		// Low Pass Filter for 3d Vectors
		double MotionCompensationManager::DEMA(const double RawData, int Axis)
		{			
			_FilterOld_vecPosition_1.v[Axis] += _alpha * (RawData - _FilterOld_vecPosition_1.v[Axis]);
			_FilterOld_vecPosition_2.v[Axis] += _alpha * (_FilterOld_vecPosition_1.v[Axis] - _FilterOld_vecPosition_2.v[Axis]);
			return 2 * _FilterOld_vecPosition_1.v[Axis] - _FilterOld_vecPosition_2.v[Axis];
		}

		// Low Pass Filter for 3d Vectors
		vr::HmdVector3d_t MotionCompensationManager::LPF(const double RawData[3], vr::HmdVector3d_t SmoothData)
		{
			vr::HmdVector3d_t RetVal;

			RetVal.v[0] = SmoothData.v[0] - (LPF_Beta * (SmoothData.v[0] - RawData[0]));
			RetVal.v[1] = SmoothData.v[1] - (LPF_Beta * (SmoothData.v[1] - RawData[1]));
			RetVal.v[2] = SmoothData.v[2] - (LPF_Beta * (SmoothData.v[2] - RawData[2]));

			return RetVal;
		}

		// Low Pass Filter for 3d Vectors
		vr::HmdVector3d_t MotionCompensationManager::LPF(vr::HmdVector3d_t RawData, vr::HmdVector3d_t SmoothData)
		{
			vr::HmdVector3d_t RetVal;

			RetVal.v[0] = SmoothData.v[0] - (LPF_Beta * (SmoothData.v[0] - RawData.v[0]));
			RetVal.v[1] = SmoothData.v[1] - (LPF_Beta * (SmoothData.v[1] - RawData.v[1]));
			RetVal.v[2] = SmoothData.v[2] - (LPF_Beta * (SmoothData.v[2] - RawData.v[2]));

			return RetVal;
		}

		// Low Pass Filter for quaternion
		vr::HmdQuaternion_t MotionCompensationManager::lowPassFilterQuaternion(vr::HmdQuaternion_t RawData, vr::HmdQuaternion_t SmoothData)
		{
			return Slerp(SmoothData, RawData, LPF_Beta);
		}

		// Spherical Linear Interpolation for Quaternions
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

		// Convert Quaternion to Euler Angles in Radians
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

		// Returns the shortest difference between to angles
		const double MotionCompensationManager::AngleDifference(double Raw, double New)
		{
			double diff = fmod((New - Raw + (double)180), (double)360) - (double)180;
			return diff < -(double)180 ? diff + (double)360 : diff;
		}

		vr::HmdVector3d_t MotionCompensationManager::Transform(vr::HmdVector3d_t VecRotation, vr::HmdVector3d_t VecPosition, vr::HmdVector3d_t point)
		{
			// point is the user-input offset to the controller
			// VecRotation and VecPosition is the current reference-pose (Controller or input from Mover)
			vr::HmdQuaternion_t quat = vrmath::quaternionFromYawPitchRoll(VecRotation.v[0], VecRotation.v[1], VecRotation.v[2]);
			Transform(quat, VecPosition, point);
		}

		// Calculates the new coordinates of 'point', moved and rotated by VecRotation and VecPosition
		vr::HmdVector3d_t MotionCompensationManager::Transform(vr::HmdQuaternion_t quat, vr::HmdVector3d_t VecPosition, vr::HmdVector3d_t point)
		{			
			vr::HmdVector3d_t translation = vrmath::quaternionRotateVector(quat, VecPosition);

			return vrmath::quaternionRotateVector(quat, point) + translation;
		}

		// 
		vr::HmdVector3d_t MotionCompensationManager::Transform(vr::HmdVector3d_t VecRotation, vr::HmdVector3d_t VecPosition, vr::HmdVector3d_t centerOfRotation, vr::HmdVector3d_t point)
		{
			// point is the user-input offset to the controller
			// VecRotation and VecPosition is the current rig-pose

			vr::HmdQuaternion_t quat = vrmath::quaternionFromYawPitchRoll(VecRotation.v[0], VecRotation.v[1], VecRotation.v[2]);

			double n1 = quat.x * 2.f;
			double n2 = quat.y * 2.f;
			double n3 = quat.z * 2.f;

			double _n4 = quat.x * n1;
			double _n5 = quat.y * n2;
			double _n6 = quat.z * n3;
			double _n7 = quat.x * n2;
			double _n8 = quat.x * n3;
			double _n9 = quat.y * n3;
			double _n10 = quat.w * n1;
			double _n11 = quat.w * n2;
			double _n12 = quat.w * n3;

			vr::HmdVector3d_t translation = {
				(1 - (_n5 + _n6)) * (VecPosition.v[0]) + (_n7 - _n12) * (VecPosition.v[1]) + (_n8 + _n11) * (VecPosition.v[2]),
				(_n7 + _n12) * (VecPosition.v[0]) + (1 - (_n4 + _n6)) * (VecPosition.v[1]) + (_n9 - _n10) * (VecPosition.v[2]),
				(_n8 - _n11) * (VecPosition.v[0]) + (_n9 + _n10) * (VecPosition.v[1]) + (1 - (_n4 + _n5)) * (VecPosition.v[2])
			};

			return {
				(1.0 - (_n5 + _n6)) * (point.v[0] - centerOfRotation.v[0]) + (_n7 - _n12) * (point.v[1] - centerOfRotation.v[1]) + (_n8 + _n11) * (point.v[2] - centerOfRotation.v[2]) + centerOfRotation.v[0] + translation.v[0],
				(_n7 + _n12) * (point.v[0] - centerOfRotation.v[0]) + (1.0 - (_n4 + _n6)) * (point.v[1] - centerOfRotation.v[1]) + (_n9 - _n10) * (point.v[2] - centerOfRotation.v[2]) + centerOfRotation.v[1] + translation.v[1],
				(_n8 - _n11) * (point.v[0] - centerOfRotation.v[0]) + (_n9 + _n10) * (point.v[1] - centerOfRotation.v[1]) + (1.0 - (_n4 + _n5)) * (point.v[2] - centerOfRotation.v[2]) + centerOfRotation.v[2] + translation.v[2]
			};
		}
	}
}