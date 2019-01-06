#pragma once

#include <openvr.h>

vr::HmdVector3_t quarternionToVector3(vr::HmdMatrix34_t matrix)
{
	vr::HmdVector3_t vector;

	vector.v[0] = matrix.m[0][3];
	vector.v[1] = matrix.m[1][3];
	vector.v[2] = matrix.m[2][3];

	return vector;
}

float getHeight(vr::TrackedDevicePose_t pTrackedDevicePose) // in meter
{
	return pTrackedDevicePose.mDeviceToAbsoluteTracking.m[1][3];
}

float getHeight(vr::TrackedDevicePose_t* pTrackedDevicePose) // in meter
{
	return pTrackedDevicePose->mDeviceToAbsoluteTracking.m[1][3];
}

float getHeight(vr::HmdMatrix34_t matrix)  // in meter
{
	return matrix.m[1][3];
}

float getHeight(vr::HmdMatrix34_t* matrix)  // in meter
{
	return matrix->m[1][3];
}

float map_numeric(float x, float in_min, float in_max, float out_min, float out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

const vr::HmdVector3_t vectorFromHeight(float height)
{
	const vr::HmdVector3_t vector = { 0.0, height, 0.0 };;

	return vector;
}

const vr::HmdVector3d_t vectorFromHeightd(float height)
{
	const vr::HmdVector3d_t vector = { 0.0, height, 0.0 };;

	return vector;
}
