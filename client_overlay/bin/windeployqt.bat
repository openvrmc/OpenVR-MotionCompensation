
cd win64

C:\Qt\Qt5.12.12\5.12.12\msvc2017_64\bin\windeployqt.exe --dir qtdata --libdir . --plugindir qtdata/plugins --no-system-d3d-compiler --compiler-runtime --no-opengl-sw --qmldir res/qml --release OpenVR-MotionCompensationOverlay.exe

@C:\Qt\Qt5.12.12\5.12.12\msvc2017_64\bin\windeployqt.exe --dir qtdata --libdir . --plugindir qtdata/plugins --qmldir res/qml OpenVR-MotionCompensationOverlay.exe --release

@REM Debug:
@REM windeployqt.exe --dir qtdata --libdir . --plugindir qtdata/plugins --no-system-d3d-compiler --compiler-runtime --no-opengl-sw --debug  --qmldir res/qml OpenVR-MotionCompensationOverlay.exe
