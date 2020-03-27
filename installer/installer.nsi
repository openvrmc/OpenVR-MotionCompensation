;--------------------------------
;Include Modern UI

	!include "MUI2.nsh"

;--------------------------------
;General

	!define OPENVR_BASEDIR "..\openvr\"
	!define OVERLAY_BASEDIR "..\client_overlay\bin\win64"
	!define DRIVER_BASEDIR "..\driver_vrmotioncompensation"

	;Name and file
	Name "OpenVR Motion Compensation"
	OutFile "OpenVR-MotionCompensation.exe"
	
	;Default installation folder
	InstallDir "$PROGRAMFILES64\OpenVR-MotionCompensation"
	
	;Get installation folder from registry if available
	InstallDirRegKey HKLM "Software\OpenVR-MotionCompensation\Overlay" ""
	
	;Request application privileges for Windows Vista
	RequestExecutionLevel admin
	
;--------------------------------
;Variables

VAR upgradeInstallation

;--------------------------------
;Interface Settings

	!define MUI_ABORTWARNING

;--------------------------------
;Pages

	!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
	!define MUI_PAGE_CUSTOMFUNCTION_PRE dirPre
	!insertmacro MUI_PAGE_DIRECTORY
	!insertmacro MUI_PAGE_INSTFILES
  
	!insertmacro MUI_UNPAGE_CONFIRM
	!insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Languages
 
	!insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Macros

;--------------------------------
;Functions

Function dirPre
	StrCmp $upgradeInstallation "true" 0 +2 
		Abort
FunctionEnd

Function .onInit
	StrCpy $upgradeInstallation "false"
 
	ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenVRMotionCompensation" "UninstallString"
	StrCmp $R0 "" done
	
	
	; If SteamVR is already running, display a warning message and exit
	FindWindow $0 "Qt5QWindowIcon" "SteamVR Status"
	StrCmp $0 0 +3
		MessageBox MB_OK|MB_ICONEXCLAMATION \
			"SteamVR is still running. Cannot install this software.$\nPlease close SteamVR and try again."
		Abort
 
	
	MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
		"OpenVR Motion Compensation is already installed. $\n$\nClick `OK` to upgrade the \
		existing installation or `Cancel` to cancel this upgrade." \
		IDOK upgrade
	Abort
 
	upgrade:
		StrCpy $upgradeInstallation "true"
	done:
FunctionEnd

;--------------------------------
;Installer Sections

Section "Install" SecInstall
	
	StrCmp $upgradeInstallation "true" 0 noupgrade 
		DetailPrint "Uninstall previous version..."
		ExecWait '"$INSTDIR\Uninstall.exe" /S _?=$INSTDIR'
		Delete $INSTDIR\Uninstall.exe
		Goto afterupgrade
		
	noupgrade:

	afterupgrade:

	SetOutPath "$INSTDIR"

	;ADD YOUR OWN FILES HERE...
	File "${OVERLAY_BASEDIR}\LICENSE"
	File "${OVERLAY_BASEDIR}\*.exe"
	File "${OVERLAY_BASEDIR}\*.dll"
	File "${OVERLAY_BASEDIR}\*.bat"
	File "${OVERLAY_BASEDIR}\*.vrmanifest"
	File "${OVERLAY_BASEDIR}\*.conf"
	File /r "${OVERLAY_BASEDIR}\res"
	File /r "${OVERLAY_BASEDIR}\qtdata"
	File "${OPENVR_BASEDIR}\bin\win64\*.dll"

	; Install redistributable
	ExecWait '"$INSTDIR\VC_redist.x64.exe" /install /quiet'
	
	Var /GLOBAL vrRuntimePath
	nsExec::ExecToStack '"$INSTDIR\OpenVR-MotionCompensationOverlay.exe" -openvrpath'
	Pop $0
	Pop $vrRuntimePath
	DetailPrint "VR runtime path: $vrRuntimePath"

	SetOutPath "$vrRuntimePath\drivers\00vrmotioncompensation"
	File "${DRIVER_BASEDIR}\driver.vrdrivermanifest"
	SetOutPath "$vrRuntimePath\drivers\00vrmotioncompensation\resources"
	File "${DRIVER_BASEDIR}\resources\driver.vrresources"
	SetOutPath "$vrRuntimePath\drivers\00vrmotioncompensation\resources\settings"
	File "${DRIVER_BASEDIR}\resources\settings\default.vrsettings"
	SetOutPath "$vrRuntimePath\drivers\00vrmotioncompensation\resources\sounds"
	File "${DRIVER_BASEDIR}\resources\sounds\audiocue.wav"
	File "${DRIVER_BASEDIR}\resources\sounds\License.txt"
	SetOutPath "$vrRuntimePath\drivers\00vrmotioncompensation\bin\win64"
	File "${DRIVER_BASEDIR}\bin\x64\driver_00vrmotioncompensation.dll"
	
	; Install the vrmanifest
	nsExec::ExecToLog '"$INSTDIR\OpenVR-MotionCompensationOverlay.exe" -installmanifest'
	
	; Post-installation step
	nsExec::ExecToLog '"$INSTDIR\OpenVR-MotionCompensationOverlay.exe" -postinstallationstep'
  
	;Store installation folder
	WriteRegStr HKLM "Software\OpenVR-MotionCompensation\Overlay" "" $INSTDIR
	WriteRegStr HKLM "Software\OpenVR-MotionCompensation\Driver" "" $vrRuntimePath
  
	;Create uninstaller
	WriteUninstaller "$INSTDIR\Uninstall.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenVRMotionCompensation" "DisplayName" "OpenVR Motion Compensation"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenVRMotionCompensation" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""

SectionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"
	; If SteamVR is already running, display a warning message and exit
	FindWindow $0 "Qt5QWindowIcon" "SteamVR Status"
	StrCmp $0 0 +3
		MessageBox MB_OK|MB_ICONEXCLAMATION \
			"SteamVR is still running. Cannot uninstall this software.$\nPlease close SteamVR and try again."
		Abort

	; Remove the vrmanifest
	nsExec::ExecToLog '"$INSTDIR\OpenVR-MotionCompensationOverlay.exe" -removemanifest'

	; Delete installed files
	Var /GLOBAL vrRuntimePath2
	ReadRegStr $vrRuntimePath2 HKLM "Software\OpenVR-MotionCompensation\Driver" ""
	DetailPrint "VR runtime path: $vrRuntimePath2"
	Delete "$vrRuntimePath2\drivers\00vrmotioncompensation\driver.vrdrivermanifest"
	Delete "$vrRuntimePath2\drivers\00vrmotioncompensation\resources\driver.vrresources"
	Delete "$vrRuntimePath2\drivers\00vrmotioncompensation\resources\settings\default.vrsettings"
	Delete "$vrRuntimePath2\drivers\00vrmotioncompensation\resources\sounds\audiocue.wav"
	Delete "$vrRuntimePath2\drivers\00vrmotioncompensation\resources\sounds\License.txt"
	Delete "$vrRuntimePath2\drivers\00vrmotioncompensation\bin\win64\driver_00vrmotioncompensation.dll"
	Delete "$vrRuntimePath2\drivers\00vrmotioncompensation\bin\win64\driver_vrmotioncompensation.log"
	RMdir "$vrRuntimePath2\drivers\00vrmotioncompensation\resources\settings"
	RMdir "$vrRuntimePath2\drivers\00vrmotioncompensation\resources\sounds"
	RMdir "$vrRuntimePath2\drivers\00vrmotioncompensation\resources\"
	RMdir "$vrRuntimePath2\drivers\00vrmotioncompensation\bin\win64\"
	RMdir "$vrRuntimePath2\drivers\00vrmotioncompensation\bin\"
	RMdir "$vrRuntimePath2\drivers\00vrmotioncompensation\"
	
	!include uninstallFiles.list

	DeleteRegKey HKLM "Software\OpenVR-MotionCompensation\Overlay"
	DeleteRegKey HKLM "Software\OpenVR-MotionCompensation\Driver"
	DeleteRegKey HKLM "Software\OpenVR-MotionCompensation"
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenVRMotionCompensation"
SectionEnd

