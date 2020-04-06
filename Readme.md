![language](https://img.shields.io/badge/Language-C%2B%2B11-green.svg)  ![dependencies](https://img.shields.io/badge/Dependencies-Boost%201.63-green.svg)  ![license_gpl3](https://img.shields.io/badge/License-GPL%203.0-green.svg)

# Beta Version, may cause crashes or contain bugs!

We are very much in development mode. If you are interested in testing or are interested in being a maintainer, please connect with us on the [discord `development` channel](https://discord.gg/r7krmSd)

# OpenVR-MotionCompensation

An OpenVR driver that allows to enable motion compensation.
Includes a dashboard to configure the settings directly in VR.

This driver hooks into the device driver and allows to modify any pose updates coming from the HMD before they reach the OpenVR runtime. 
Due to the nature of this hack the driver may break when Valve decides to update the driver-side OpenVR API.

The motivation of this driver is to allow the base of motion simulators (driving or flying) to be the reference point for the world, cancelling out simulator movement and differencing head movement from the simulator movement to update the pose.


# Notes:

This is a work-in-progress and may contain bugs.


## How to open

There are two ways to open the settings page:
1. On VR, open the Steam Dashboard (Menu button on the controller). In the bottom left is a new icon, a small cogwheel. Click on this icon to bring up the OVRMC overlay 
2. Go to the install folder and click on "startdesktopmode.bat". This will open OVRMC on your regular desktop.


## Top Page:

![Root Page](docs/screenshots/DeviceManipulationPage.png)

- **HMD**: Choose the HMD you use (usaly only one should appear)
- **Status**: Shows the current status of the selected HMD.
- **Reference Tracker**: Choose the tracker / controller you want to use as reference.
- **Status**: Shows the current status of the selected tracker / controller.
- **Identify**: Sends a haptic pulse to the selected device (not yet implemented)
- **Enable Motion Compensation**: Enable motion compensation with the selected devices.
- **LPF Beta value**: Filter setting. Need to be between 0 and 1. Use the +- Buttons to incread / decrease in 0.05 Steps.
- **Apply**: Apply the choosen settings.


## LPF Beta value:

Values must be between 0 and 1. A value closer to 0 means a stronger filter. A value clsoer to 1 means a weaker filter.
If the value is to low, you may expirience a slow drifting. Raise the value for a faster response.
Default value is set to 0.2


## Mounting advice:

Mount the reference tracker/controller as closely to the head position as possible. The further away it is from the head position the larger the error.
Continuous and heavy vibration will affect the IMU performance, causing noticable IMU drift. This will be noticable in the HMD in form of your camera moving violently arround the car. To adress this, it is suggested to use a damping system.


## Initial Setup
See the wiki

## Building
1. Open *'VRMotionCompensation.sln'* in Visual Studio 2019.
2. Build Solution

# License

This software is released under GPL 3.0.