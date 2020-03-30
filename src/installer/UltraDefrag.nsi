/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2012 Dmitri Arkhangelski (dmitriar@gmail.com).
 *  Copyright (c) 2010-2012 Stefan Pendl (stefanpe@users.sourceforge.net).
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
* UltraDefrag regular installer.
*/

/*
*  NOTE: The following symbols must be
*  defined through makensis command line:
*  ULTRADFGVER=<version number in form x.y.z>
*  ULTRADFGARCH=<i386 | amd64 | ia64>
*  UDVERSION_SUFFIX
*  The following symbol is accepted too,
*  but may be undefined:
*  RELEASE_STAGE
*/

!ifndef ULTRADFGVER
!error "ULTRADFGVER parameter must be specified on the command line!"
!endif

!ifndef ULTRADFGARCH
!error "ULTRADFGARCH parameter must be specified on the command line!"
!endif

!ifndef UDVERSION_SUFFIX
!error "UDVERSION_SUFFIX parameter must be specified on the command line!"
!endif

/*
 * Constants
 */

!define UD_UNINSTALL_REG_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\UltraDefrag"
!define UD_LOG_FILE "$TEMP\UltraDefrag_Install.log"

!if ${ULTRADFGARCH} == 'i386'
!define ROOTDIR "..\.."
!else
!define ROOTDIR "..\..\.."
!endif

/*
 * Compress installer exehead with an executable compressor
 */

!packhdr temp.dat '"${ROOTDIR}\src\tools\upx\upx" --best -q temp.dat'

/*
 * Include additional plug-in folders
 */

!addplugindir "${ROOTDIR}\src\installer\plug_ins\Plugin"
!addincludedir "${ROOTDIR}\src\installer\plug_ins\Include"

/*
 * Installer Attributes
 */

!ifdef RELEASE_STAGE
    !if ${ULTRADFGARCH} == 'amd64'
    Name "Ultra Defragmenter v${ULTRADFGVER} ${RELEASE_STAGE} (AMD64)"
    !else if ${ULTRADFGARCH} == 'ia64'
    Name "Ultra Defragmenter v${ULTRADFGVER} ${RELEASE_STAGE} (IA64)"
    !else
    Name "Ultra Defragmenter v${ULTRADFGVER} ${RELEASE_STAGE} (i386)"
    !endif
!else
    !if ${ULTRADFGARCH} == 'amd64'
    Name "Ultra Defragmenter v${ULTRADFGVER} (AMD64)"
    !else if ${ULTRADFGARCH} == 'ia64'
    Name "Ultra Defragmenter v${ULTRADFGVER} (IA64)"
    !else
    Name "Ultra Defragmenter v${ULTRADFGVER} (i386)"
    !endif
!endif

!if ${ULTRADFGARCH} == 'i386'
InstallDir "$PROGRAMFILES\UltraDefrag"
!else
InstallDir "$PROGRAMFILES64\UltraDefrag"
!endif

InstallDirRegKey HKLM ${UD_UNINSTALL_REG_KEY} "InstallLocation"

Var OldInstallDir
Var ValidDestDir

OutFile "ultradefrag-${UDVERSION_SUFFIX}.bin.${ULTRADFGARCH}.exe"
LicenseData "${ROOTDIR}\src\LICENSE.TXT"
ShowInstDetails show
ShowUninstDetails show

Icon "${ROOTDIR}\src\installer\udefrag-install.ico"
UninstallIcon "${ROOTDIR}\src\installer\udefrag-uninstall.ico"

XPStyle on
RequestExecutionLevel admin

InstType "Full"
InstType "Micro Edition"

/*
 * Compiler Flags
 */

AllowSkipFiles off
SetCompressor /SOLID lzma

/*
 * Version Information
 */

VIProductVersion "${ULTRADFGVER}.0"
VIAddVersionKey  "ProductName"     "Ultra Defragmenter"
VIAddVersionKey  "CompanyName"     "UltraDefrag Development Team"
VIAddVersionKey  "LegalCopyright"  "Copyright © 2007-2012 UltraDefrag Development Team"
VIAddVersionKey  "FileDescription" "Ultra Defragmenter Setup"
VIAddVersionKey  "FileVersion"     "${ULTRADFGVER}"

/*
 * Headers
 */

!include "WinMessages.nsh"
!include "WinVer.nsh"
!include "x64.nsh"
!include "MUI.nsh"
!include "${ROOTDIR}\src\installer\UltraDefrag.nsh"
!include "${ROOTDIR}\src\installer\LanguageSelector.nsh"
!include "${ROOTDIR}\src\installer\PresetSections.nsh"

/*
 * Modern User Interface Pages
 */

!define MUI_ICON   "${ROOTDIR}\src\installer\udefrag-install.ico"
!define MUI_UNICON "${ROOTDIR}\src\installer\udefrag-uninstall.ico"

!define MUI_WELCOMEFINISHPAGE_BITMAP   "${ROOTDIR}\src\installer\WelcomePageBitmap.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${ROOTDIR}\src\installer\WelcomePageBitmap.bmp"
!define MUI_COMPONENTSPAGE_SMALLDESC

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${ROOTDIR}\src\LICENSE.TXT"
!define MUI_DIRECTORYPAGE_TEXT_TOP "Only empty folders and folders containing a previous UltraDefrag installation are valid!$\n \
    For any other folders the $\"Next$\" button will be disabled."
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro LANG_PAGE
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_NOAUTOCLOSE
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

/*
 * Component Sections
 */

Section "Ultra Defrag core files (required)" SecCore

    SectionIn 1 2 RO

    ${InstallCoreFiles}

SectionEnd

SectionGroup /e "Interfaces (at least one must be selected)" SecInterfaces

Section "Boot" SecBoot

    SectionIn 1 2

    ${InstallBootFiles}

SectionEnd

Section "Console" SecConsole

    SectionIn 1 2

    ${InstallConsoleFiles}

SectionEnd

Section "GUI (Default)" SecGUI

    SectionIn 1

    ${InstallGUIFiles}

SectionEnd

SectionGroupEnd

Section "Documentation" SecHelp

    SectionIn 1

    ${InstallHelpFiles}

SectionEnd

Section "Context menu handler (requires Console)" SecShellHandler

    SectionIn 1 2

    ${InstallShellHandlerFiles}

SectionEnd

SectionGroup /e "Shortcuts (require GUI)" SecShortcuts

Section "Start Menu icon" SecStartMenuIcon

    SectionIn 1

    ${InstallStartMenuIcon}

SectionEnd

Section "Desktop icon" SecDesktopIcon

    SectionIn 1

    ${InstallDesktopIcon}

SectionEnd

Section "Quick Launch icon" SecQuickLaunchIcon

    SectionIn 1

    ${InstallQuickLaunchIcon}

SectionEnd

SectionGroupEnd

Section "Turn off usage tracking" SecUsageTracking

    ${InstallUsageTracking}

SectionEnd

;----------------------------------------------

Section "Uninstall"

    ${DisableX64FSRedirection}

    DetailPrint "Deregister boot time defragmenter..."
    ExecWait '"$SYSDIR\bootexctrl.exe" /u /s defrag_native'
    Delete "$WINDIR\pending-boot-off"

    ${EnableX64FSRedirection}

    ${RemoveQuickLaunchIcon}
    ${RemoveDesktopIcon}
    ${RemoveStartMenuIcon}
    ${RemoveUsageTracking}
    ${RemoveShellHandlerFiles}
    ${RemoveHelpFiles}
    ${RemoveGUIFiles}
    ${RemoveConsoleFiles}
    ${RemoveBootFiles}
    ${RemoveCoreFiles}

SectionEnd

;----------------------------------------------

Function .onInit

    ; remove old log file
    Delete ${UD_LOG_FILE}
    ClearErrors

    ${CheckAdminRights}
    ${CheckWinVersion}
    ${CheckMutex}

    ${EnableX64FSRedirection}
    InitPluginsDir

    ${DisableX64FSRedirection}

    ReadRegStr $OldInstallDir HKLM ${UD_UNINSTALL_REG_KEY} "InstallLocation"
    StrCmp $OldInstallDir "" 0 +2
    StrCpy $OldInstallDir $INSTDIR

    ${CollectOldLang}
    ${InitLanguageSelector}

    ${CollectFromRegistry}
    ${ParseCommandLine}

    ${EnableX64FSRedirection}

FunctionEnd

Function un.onInit

    ${CheckAdminRights}
    ${CheckMutex}

FunctionEnd

;----------------------------------------------

Function .onSelChange

    ${VerifySelections}

FunctionEnd

;----------------------------------------------

Function .onVerifyInstDir

    ${CheckDestFolder}

    StrCmp $ValidDestDir "1" +2
    Abort

FunctionEnd

;----------------------------------------------

Function .onInstSuccess

    ${PreserveInRegistry}

    ${WriteTheUninstaller}

    ${RemoveObsoleteFiles}

    ${UpdateUninstallSizeValue}

    ${RegisterInstallationFolder}

    ${InitCrashDate}

FunctionEnd

Function un.onUninstSuccess

    DetailPrint "Removing installation directory..."
    ; safe, because installation directory is predefined
    RMDir /r $INSTDIR

    DetailPrint "Cleanup registry..."
    DeleteRegKey HKLM ${UD_UNINSTALL_REG_KEY}

    ${UnRegisterInstallationFolder}

FunctionEnd

;----------------------------------------------

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCore}            "The core files required to use UltraDefrag."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecInterfaces}      "Select at least one interface to be installed."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecBoot}            "The boot time interface processes your volumes during the start of Windows."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecConsole}         "The command line interface can be used for automation."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecGUI}             "The graphical user interface with cluster map and volume list."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecHelp}            "Handbook, documentation or help file however you like to call it."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecShellHandler}    "Defragment your volumes, folders and files from their Explorer context menu."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcuts}       "Adds icons for easy access."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecStartMenuIcon}   "Adds an icon to your start menu."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktopIcon}     "Adds an icon to your desktop."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecQuickLaunchIcon} "Adds an icon to your quick launch toolbar."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecUsageTracking}   "Disable web based usage tracking."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;---------------------------------------------

/* EOF */
