
cd win64

windeployqt.exe --dir qtdata --libdir . --plugindir qtdata/plugins --no-system-d3d-compiler --no-opengl-sw --release  --qmldir res/qml OpenVR-InputEmulatorOverlay.exe
C:\Qt\Qt5.13.0\5.13.0\msvc2017_64\bin\windeployqt.exe --dir qtdata --libdir . --plugindir qtdata/plugins --qmldir res/qml OpenVR-InputEmulatorOverlay.exe

@REM Debug:
@REM windeployqt.exe --dir qtdata --libdir . --plugindir qtdata/plugins --no-system-d3d-compiler --compiler-runtime --no-opengl-sw --debug  --qmldir res/qml OpenVR-InputEmulatorOverlay.exe
