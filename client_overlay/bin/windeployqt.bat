
cd win64

C:\local\Qt\5.15.2\msvc2019_64\bin\windeployqt.exe --dir qtdata --libdir . --plugindir qtdata/plugins --no-system-d3d-compiler --compiler-runtime --no-opengl-sw --qmldir res/qml --release OpenVR-MotionCompensationOverlay.exe

@C:\local\Qt\5.15.2\msvc2019_64\bin\windeployqt.exe --dir qtdata --libdir . --plugindir qtdata/plugins --qmldir res/qml OpenVR-MotionCompensationOverlay.exe --release

@REM Debug:
@REM windeployqt.exe --dir qtdata --libdir . --plugindir qtdata/plugins --no-system-d3d-compiler --compiler-runtime --no-opengl-sw --debug  --qmldir res/qml OpenVR-MotionCompensationOverlay.exe
