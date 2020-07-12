![language](https://img.shields.io/badge/Language-C%2B%2B11-green.svg)  ![dependencies](https://img.shields.io/badge/Dependencies-Boost%201.72-green.svg)  ![license_gpl3](https://img.shields.io/badge/License-GPL%203.0-green.svg)

# Beta Version, may cause crashes or contain bugs!

If you are interested in testing or are interested in being a maintainer, please connect with us on our [discord server](https://discord.gg/r7krmSd)

# OpenVR-MotionCompensation

An OpenVR driver that allows to enable motion compensation.
Includes a dashboard to configure the settings directly in VR.

This driver hooks into the device driver and allows to modify any pose updates coming from the HMD before they reach the OpenVR runtime. 
Due to the nature of this hack the driver may break when Valve decides to update the driver-side OpenVR API.

The motivation of this driver is to allow the base of motion simulators (driving or flying) to be the reference point for the world, cancelling out simulator movement and differencing head movement from the simulator movement to update the pose.


# Notes

This is a work-in-progress and may contain bugs.


## Current development focus:
- Fix "Identify" not working


# How to use

## Tracker Mounting:

Mount the reference tracker/controller somewhere firmly onto your simulator. The position does not matter, but the controller must be placed where it is well tracked.
Continuous and heavy vibration will affect the IMU performance, causing noticable IMU drift. This will be noticable in the HMD in form of your camera moving violently arround. To adress this, it is suggested to use a damping system.<br>


## Download and Install
Download the latest version and install it. No additional steps needed!<br>
Windows Defender might pop up with "unknown publisher" message. If you don't want to take this risk, you can of course compile it from source by yourself.


## Open settings

There are two ways to open the settings page:
1. In VR, open the Steam Dashboard (Menu button on the controller). In the bottom left is a new icon, a small cogwheel. Click on this icon to bring up the OVRMC overlay
2. Go to the install folder and click on "startdesktopmode.bat". This will open OVRMC on your regular desktop.


## Top Page:

![Root Page](docs/screenshots/DeviceManipulationPage.png)

- **HMD**: Choose the HMD you use (usaly only one should appear)
- **Status**: Shows the current status of the selected HMD.
- **Reference Tracker**: Choose the tracker / controller you want to use as reference.
- **Status**: Shows the current status of the selected tracker / controller.
- **Identify**: Sends a haptic pulse to the selected device. (Does not work in current version)
- **Enable Motion Compensation**: Enable motion compensation with the selected devices.
- **Settings**: Opens the Settings Page
- **Apply**: Apply and save the choosen settings. Also sets the reference pose.


## Settings Page:
- **Motion Compensation mode**: Choose the motion compensation mode. Currently there is only one.
- **LPF Beta value**: Filter setting for rotation. Needs to be between 0 and 1. Set to 1 to disable. Use the +- Buttons to incread / decrease in 0.05 steps.
- **Samples**: Filter setting for xyz. Use the +- Buttons to incread / decrease by 5.
- **Set Zero**: Set all Velocity and Acceleration values to zero that are send to the HMD. This might help with rendering issues, specialy with Oculus and WMR HMDs.

### Offsets for virtual Driver:
They are only used to position a virtual 3rd-party driver. They have no other effect. Don't bother to set them if you are not using an virutal device driver!

###
Keyboard hotkey
- **Enable / Disable MC:** Toogles the motion compensation on and off.
- **Reset Reference Pose:** As the name says.

Note: Due to the inner workings of OpenVR you have to set the Hotkeys with the desktop mode! It will not work in VR!

# Filter settings

## LPF Beta value:

Values must be between 0 and 1. The number is the rate at which the filter follows a new value. A '1' means immediate adoption of the value. Where a '0' means no adoption.<br>
A low filter-value will filter vibrations better, but you will notice a swimming or lagging picture, as it takes time for the filter to adapt to the real value.<br>
You have to find the sweet spot for your setup. A good starting point is 0.2 for Lighthouse-Tracked devices.<br>

## Samples:

A higher value will filter vibrations better, but you will notice a swimming or lagging picture if it is too high. Go with +-50 steps and tune from there.<br>


# License

This software is released under GPL 3.0.