![language](https://img.shields.io/badge/Language-C%2B%2B11-green.svg)  ![dependencies](https://img.shields.io/badge/Dependencies-Boost%201.72-green.svg)  ![license_gpl3](https://img.shields.io/badge/License-GPL%203.0-green.svg)

# Beta Version, may cause crashes or contain bugs!

If you are interested in testing or are interested in being a maintainer, please connect with us on our [discord server](https://discord.gg/r7krmSd)

# OpenVR-MotionCompensation

An OpenVR driver that allows to enable motion compensation.
Includes a dashboard to configure the settings directly in VR.

This driver hooks into the device driver and allows to modify any pose updates coming from the HMD before they reach the OpenVR runtime. 
Due to the nature of this hack the driver may break when Valve decides to update the driver-side OpenVR API.

The motivation of this driver is to allow the base of motion simulators (driving or flying) to be the reference point for the world, cancelling out simulator movement and differencing head movement from the simulator movement to update the pose.

Visit https://ovrmc.dschadu.de/ for more information!

# License

This software is released under GPL 3.0.