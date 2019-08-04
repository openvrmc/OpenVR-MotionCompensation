![language](https://img.shields.io/badge/Language-C%2B%2B11-green.svg)  ![dependencies](https://img.shields.io/badge/Dependencies-Boost%201.63-green.svg)  ![license_gpl3](https://img.shields.io/badge/License-GPL%203.0-green.svg)

# DO NOT USE THIS OR EXPECT ANYTHING AS A USER
I haven't coded in C++ for twenty years.  I am investigating limiting the functional/api surface area to support motion compensation only.  Feel free to contact me if you want to help out, or if you are experienced, potentially be paid to help out - https://github.com/rosskevin/OpenVR-MotionCompensation/issues/1.

---

# [Former docs] OpenVR-InputEmulator

An OpenVR driver that allows to enable motion compensatio. 
Includes a dashboard to configure some settings directly in VR.

The OpenVR driver hooks into the lighthouse driver and allows to modify any pose updates or button/axis events coming from the Vive controllers before they reach the OpenVR runtime. 
Due to the nature of this hack the driver may break when Valve decides to update the driver-side OpenVR API.

The motivation of this driver is to allow the base of motion simulators (driving or flying) to be the reference point for the world, cancelling out simulator movement and differencing
head movement from the simulator movement to update the pose.


# Notes:

This is a work-in-progress and may contain bugs.


## Top Page:

![Root Page](docs/screenshots/DeviceManipulationPage.png)

- **Identify**: Sends a haptic pulse to the selected device (Devices without haptic feedback like the Vive trackers can be identified by a flashing white light).
- **Status**: Shows the current status of the selected device.
- **Device Mode**: Allows to select a device mode.
  - **Default**: Default mode.
  - **Disable**: Let OpenVR think that the device has been disconnected.
  - **Redirect to**: Impersonate another device.
  - **Swap with**: Swap two devices.
  - **Motion Compensation**: Enable motion compensation with the selected device as reference device.
- **Device Offsets**: Allows to add translation or rotation offsets to the selected device.
- **Motion Compensation Settings**: Allows to configure motion compensation mode.
- **Render Model**: Shows a render model at the device position (experimental).
- **Input Remapping**: Allows to re-map input coming from controller buttons, joysticks or touchpads.
- **Profile**: Allows to apply/define/delete device offsets/motion compensation profiles.


## Motion Compensation Settings Page:

![Motion Compensation Settings Page](docs/screenshots/MotionCompensationPage.png)

**Vel/Acc Compensation Mode**: How should reported velocities and acceleration values be adjusted. The problem with only adjusting the headset position is that pose prediction also takes velocity and acceleration into accound. As long as the reported values to not differ too much from the real values, pose prediction errors are hardly noticeable. But with fast movements of the motion platform the pose prediction error can be noticeable. Available modes are:
- **Disabled**: Do not adjust velocity/acceration values.
- **Set Zero**: Set all velocity/acceleration values to zero. Most simple form of velocity/acceleration compensation.
- **Use Reference Tracker**: Substract the velocity/acceleration values of the motion compensation reference tracker/controller from the values reported from the headset. Most accurate form of velocity/acceleration compensation. However, it requires that the reference tracker/controller is as closely mounted to the head position as possible. The further away it is from the head position the larger the error.
- **Linear Approximation w/ Moving Average (Experimental)**: Uses linear approximation to estimate the velocity/acceleration values. The used formula is: (current_position - last_position) / time_difference. To reduce jitter the average over the last few values is used.
  - **Moving Average Window**: How many values are used for calculating the average.
- **Kalman Filter (Experimental)**: The position values are fed into a kalman filter which then outputs a velocity value. The kalman filter implementation is based on the filter described [here](https://en.wikipedia.org/wiki/Kalman_filter#Example_application.2C_technical).
  - **Process/Observation Noise**: Parameters used to fine-tune the kalman filter. 
  

## Initial Setup
See the wiki

## Building
1. Open *'VRInputEmulator.sln'* in Visual Studio 2019.
2. Build Solution

# License

This software is released under GPL 3.0.
