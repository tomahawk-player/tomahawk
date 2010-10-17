; assuming the script is in ROOT/admin/win/
!define ROOTDIR "../.."

!include "MUI2.nsh"

Name "Tomahawk"
!define MUI_NAME "Tomahawk"
!define MUI_PRODUCT "Tomahawk"
!define MUI_FILE "Tomahawk"
!define MUI_VERSION "Alpha"
!define MUI_BRANDINGTEXT "Tomahawk-Player Alpha Test"
CRCCheck On
 

 
OutFile "tomahawk-setup-alpha.exe"
;ShowInstDetails "nevershow"
ShowUninstDetails "nevershow"
;SetCompressor "bzip2"
 
!define MUI_ICON "..\..\data\icons\tomahawk.ico"
!define MUI_UNICON "..\..\data\icons\tomahawk.ico"
;!define MUI_SPECIALBITMAP "Bitmap.bmp"
 
 
InstallDir "$PROGRAMFILES\${MUI_PRODUCT}"
 
;--------------------------------
;Modern UI Configuration
!define MUI_WELCOMEPAGE_TEXT "This is an Alpha release, and is still buggy.$\n$\nPlease join #tomahawk-player on irc.freenode.net" 
!insertmacro MUI_PAGE_WELCOME

!insertmacro MUI_PAGE_LICENSE "${ROOTDIR}\LICENSE.txt"
;;!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN "$INSTDIR\tomahawk.exe"
!insertmacro MUI_PAGE_FINISH
 
!insertmacro MUI_LANGUAGE "English"

;Modern UI System
;!insertmacro MUI_SYSTEM 

LicenseData "${ROOTDIR}\LICENSE.txt"
 
Section "install"
 
;Add files
SetOutPath "$INSTDIR"

;Path to our DLL cache
!define DLLS "${ROOTDIR}\admin\win\dlls"

File "${ROOTDIR}\build\tomahawk.exe"
File "${ROOTDIR}\LICENSE.txt"

; QT stuff:  
File "${DLLS}\QtCore4.dll"
File "${DLLS}\QtGui4.dll"
File "${DLLS}\QtNetwork4.dll"
File "${DLLS}\QtSql4.dll"
File "${DLLS}\QtXml4.dll"
SetOutPath "$INSTDIR\sqldrivers"
File "${DLLS}\sqldrivers\qsqlite4.dll"
SetOutPath "$INSTDIR"

; Cygwin/c++ stuff
File "${DLLS}\cygmad-0.dll"
File "${DLLS}\libgcc_s_dw2-1.dll"
File "${DLLS}\mingwm10.dll"

; Audio stuff
File "${DLLS}\libmad.dll"
File "${DLLS}\librtaudio.dll"

; Other
File "${DLLS}\libqjson.dll"
File "${DLLS}\libqxtweb-standalone.dll"
File "${DLLS}\libtag.dll"

;create desktop shortcut
CreateShortCut "$DESKTOP\${MUI_PRODUCT}.lnk" "$INSTDIR\${MUI_FILE}.exe" ""
 
;create start-menu items
CreateDirectory "$SMPROGRAMS\${MUI_PRODUCT}"
CreateShortCut "$SMPROGRAMS\${MUI_PRODUCT}\Uninstall.lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\Uninstall.exe" 0
CreateShortCut "$SMPROGRAMS\${MUI_PRODUCT}\${MUI_PRODUCT}.lnk" "$INSTDIR\${MUI_FILE}.exe" "" "$INSTDIR\${MUI_FILE}.exe" 0
 
;write uninstall information to the registry
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "DisplayName" "${MUI_PRODUCT} (remove only)"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "UninstallString" "$INSTDIR\Uninstall.exe"
 
WriteUninstaller "$INSTDIR\Uninstall.exe"
 
SectionEnd
 
 
;--------------------------------    
;Uninstaller Section  
Section "Uninstall"
 
;Delete Files 
RMDir /r "$INSTDIR\*.*"    
 
;Remove the installation directory
RMDir "$INSTDIR"
 
;Delete Start Menu Shortcuts
Delete "$DESKTOP\${MUI_PRODUCT}.lnk"
Delete "$SMPROGRAMS\${MUI_PRODUCT}\*.*"
RmDir  "$SMPROGRAMS\${MUI_PRODUCT}"
 
;Delete Uninstaller And Unistall Registry Entries
DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${MUI_PRODUCT}"
DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}"  
 
SectionEnd
 
 
Function un.onUninstSuccess
  MessageBox MB_OK "You have successfully uninstalled ${MUI_PRODUCT}."
FunctionEnd
