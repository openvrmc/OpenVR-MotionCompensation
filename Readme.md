![language](https://img.shields.io/badge/Language-C%2B%2B11-green.svg)  ![dependencies](https://img.shields.io/badge/Dependencies-Boost%201.72-green.svg)  ![license_gpl3](https://img.shields.io/badge/License-GPL%203.0-green.svg)

# Towards a Bedder Future: A Study of Using Virtual Reality while Lying Down

This repository contains the modified code of the original OpenVR-MotionCompensation hack below.
This code was used in a research project at the University of Copenhagen that investigated the use of VR while lying down.
Our modifications allows us to rotate the virtual world based on a tracker, and also compensate the pose of the controllers (this is new compared to original project).

The paper will be available soon. This repository already provides the source code and release to use this technique yourself.

## Abstract
Most contemporary Virtual Reality (VR) experiences are made for standing users. However, when a user is lying down---either by choice or necessity---it is unclear how they can walk around, dodge obstacles, or grab distant objects. We rotate the virtual coordinate space to study the movement requirements and user experience of using VR while lying down. Fourteen experienced VR users engaged with various popular VR applications for 40 minutes in a study using a think-aloud protocol and semi-structured interviews. Thematic analysis of captured videos and interviews reveals that using VR while lying down is comfortable and usable and that the virtual perspective produces a potent illusion of standing up. However, commonplace movements in VR are surprisingly difficult when lying down, and using alternative interactions is fatiguing and hampers performance. To conclude, we discuss design opportunities to tackle the most significant challenges and to create new experiences.

## How it works
TODO

# Setup

## Beta Version, may cause crashes or contain bugs!

Confirmed Working With..

- SteamVR v1.22.13
- HTC Vive
- HTC Vive wands (2x)
- Valve Index Knuckles (2x)
- HTC Vice Tracker

## Installation

1. Run the installer v3.6.0 from the original project (https://ovrmc.dschadu.de/en/Download).
2. Copy and overwrite the code from OpenVR-MotionCompensation\client_overlay\bin\win64 to insall dir (C:\Program Files\OpenVR-MotionCompensation)
3. Copy and overwrite the DLL from OpenVR-MotionCompensation\driver_vrmotioncompensation\bin\x64 to the SteamVR driver (C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\00vrmotioncompensation\bin\win64)
4. Start SteamVR. Start MotionCompensation from either the in-game overlay or startdesktopmode.bat in the install dir.

## Building

### Dependencies
1. Boost C++ Libraries 1.72 for MSVC 14 64-bit. You can get these from https://sourceforge.net/projects/boost/
2. Qt Framework 5 with MSVC 2019 support (you may drop everything else). You can get the free Open-Source SDK from https://download.qt.io/archive/qt/5.15/5.15.8/single/
3. OpenVR headers. You can get these in the included directory or at https://github.com/ValveSoftware/openvr/

### Build requirements
1. Set Visual Studio to Release/x64 target.
2. Set up the include and additional dependency directories in Visual Studio project settings. 
3. This requires particularly to set the $(QTDIR), $(OPENVR_ROOT), $(BOOST_ROOT), and $(BOOST_LIB) macros correctly.
These are defined in the *.vcxproj files.
4. Build solution in Visual Studio (tested with VS2022 and MSVC v147)
5. Edit the path to your Qt install in ./client_overlay/bin/windeployqt.bat
6. Run windeployqt.bat
7. Copy binaries as instructed above.

# To-Do

1. Fix the offset field not updating / offsets not applying.
2. Implement rotation offsets.
3. Implement auto-offsets.
4. Investigate throwing bugs (wrong direction).
5. Update to use v3.7.0

# Original README - OpenVR-MotionCompensation

An OpenVR driver that allows to enable motion compensation.
Includes a dashboard to configure the settings directly in VR.

This driver hooks into the device driver and allows to modify any pose updates coming from the HMD before they reach the OpenVR runtime. 
Due to the nature of this hack the driver may break when Valve decides to update the driver-side OpenVR API.

The motivation of this driver is to allow the base of motion simulators (driving or flying) to be the reference point for the world, cancelling out simulator movement and differencing head movement from the simulator movement to update the pose.

Visit https://ovrmc.dschadu.de/ for more information!

# License

This software is released under GPL 3.0.