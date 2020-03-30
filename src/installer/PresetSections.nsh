/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2010-2013 Stefan Pendl (stefanpe@users.sourceforge.net).
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
* Routines to select the components based on the command line switches
* and the previous selections read from the registry,
* and to save them for the future.
*/

!ifndef _PRESET_SECTIONS_NSH_
!define _PRESET_SECTIONS_NSH_

; --- support command line parsing
!include "FileFunc.nsh"
!insertmacro GetParameters
!insertmacro GetOptions

; --- simple section handling
!include "Sections.nsh"
!define SelectSection    "!insertmacro SelectSection"
!define UnselectSection  "!insertmacro UnselectSection"
!define SetSectionFlag   "!insertmacro SetSectionFlag"
!define ClearSectionFlag "!insertmacro ClearSectionFlag"

/*
 * This collects the previous selections from the registry
 * add to .onInit
 */
!macro CollectFromRegistry

    Push $R1

    DetailPrint "Collecting previous selections from registry ..."

    ClearErrors
    ReadRegStr $R1 HKLM ${UD_UNINSTALL_REG_KEY} "Var::InstallBoot"
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecBoot}
        ${Else}
            ${UnselectSection} ${SecBoot}
        ${EndIf}
    ${EndUnless}

    ClearErrors
    ReadRegStr $R1 HKLM ${UD_UNINSTALL_REG_KEY} "Var::InstallConsole"
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecConsole}
        ${Else}
            ${UnselectSection} ${SecConsole}
        ${EndIf}
    ${EndUnless}

    ClearErrors
    ReadRegStr $R1 HKLM ${UD_UNINSTALL_REG_KEY} "Var::InstallGUI"
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecGUI}
        ${Else}
            ${UnselectSection} ${SecGUI}
        ${EndIf}
    ${EndUnless}

    ClearErrors
    ReadRegStr $R1 HKLM ${UD_UNINSTALL_REG_KEY} "Var::InstallHelp"
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecHelp}
        ${Else}
            ${UnselectSection} ${SecHelp}
        ${EndIf}
    ${EndUnless}

    ClearErrors
    ReadRegStr $R1 HKLM ${UD_UNINSTALL_REG_KEY} "Var::InstallShellHandler"
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecShellHandler}
        ${Else}
            ${UnselectSection} ${SecShellHandler}
        ${EndIf}
    ${EndUnless}

    ClearErrors
    ReadRegStr $R1 HKLM ${UD_UNINSTALL_REG_KEY} "Var::InstallUsageTracking"
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecUsageTracking}
        ${Else}
            ${UnselectSection} ${SecUsageTracking}
        ${EndIf}
    ${EndUnless}

    ClearErrors
    ReadRegStr $R1 HKLM ${UD_UNINSTALL_REG_KEY} "Var::InstallStartMenuIcon"
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecStartMenuIcon}
        ${Else}
            ${UnselectSection} ${SecStartMenuIcon}
        ${EndIf}
    ${EndUnless}

    ClearErrors
    ReadRegStr $R1 HKLM ${UD_UNINSTALL_REG_KEY} "Var::InstallDesktopIcon"
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecDesktopIcon}
        ${Else}
            ${UnselectSection} ${SecDesktopIcon}
        ${EndIf}
    ${EndUnless}

    ClearErrors
    ReadRegStr $R1 HKLM ${UD_UNINSTALL_REG_KEY} "Var::InstallQuickLaunchIcon"
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecQuickLaunchIcon}
        ${Else}
            ${UnselectSection} ${SecQuickLaunchIcon}
        ${EndIf}
    ${EndUnless}

    Pop $R1

!macroend

!define CollectFromRegistry "!insertmacro CollectFromRegistry"

;-----------------------------------------------------------

/*
 * This collects selections from the command line
 * add to .onInit
 */
!macro ParseCommandLine

    Push $R0
    Push $R1

    DetailPrint "Collecting selections from the command line ..."
    ${GetParameters} $R0

    ClearErrors
    ${GetOptions} $R0 /FULL= $R1
    ${Unless} ${Errors}
        SetCurInstType 0
    ${EndUnless}

    ClearErrors
    ${GetOptions} $R0 /MICRO= $R1
    ${Unless} ${Errors}
        SetCurInstType 1
    ${EndUnless}

    ClearErrors
    ${GetOptions} $R0 /ICONS= $R1
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecShortcuts}
        ${Else}
            ${UnselectSection} ${SecShortcuts}
        ${EndIf}
    ${EndUnless}

    ClearErrors
    ${GetOptions} $R0 /BOOT= $R1
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecBoot}
        ${Else}
            ${UnselectSection} ${SecBoot}
        ${EndIf}
    ${EndUnless}

    ClearErrors
    ${GetOptions} $R0 /CONSOLE= $R1
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecConsole}
        ${Else}
            ${UnselectSection} ${SecConsole}
            ${UnselectSection} ${SecShellHandler}
        ${EndIf}
    ${EndUnless}

    ClearErrors
    ${GetOptions} $R0 /GUI= $R1
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecGUI}
        ${Else}
            ${UnselectSection} ${SecGUI}
            ${UnselectSection} ${SecShortcuts}
        ${EndIf}
    ${EndUnless}

    ClearErrors
    ${GetOptions} $R0 /HELP= $R1
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecHelp}
        ${Else}
            ${UnselectSection} ${SecHelp}
        ${EndIf}
    ${EndUnless}

    ClearErrors
    ${GetOptions} $R0 /SHELLEXTENSION= $R1
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecShellHandler}
            ${SelectSection} ${SecConsole}
        ${Else}
            ${UnselectSection} ${SecShellHandler}
        ${EndIf}
    ${EndUnless}

    ClearErrors
    ${GetOptions} $R0 /DISABLE_USAGE_TRACKING= $R1
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecUsageTracking}
        ${Else}
            ${UnselectSection} ${SecUsageTracking}
        ${EndIf}
    ${EndUnless}

    ClearErrors
    ${GetOptions} $R0 /STARTMENUICON= $R1
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecStartMenuIcon}
            ${SelectSection} ${SecGUI}
        ${Else}
            ${UnselectSection} ${SecStartMenuIcon}
        ${EndIf}
    ${EndUnless}

    ClearErrors
    ${GetOptions} $R0 /DESKTOPICON= $R1
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecDesktopIcon}
            ${SelectSection} ${SecGUI}
        ${Else}
            ${UnselectSection} ${SecDesktopIcon}
        ${EndIf}
    ${EndUnless}

    ClearErrors
    ${GetOptions} $R0 /QUICKLAUNCHICON= $R1
    ${Unless} ${Errors}
        ${If} $R1 == "1"
            ${SelectSection} ${SecQuickLaunchIcon}
            ${SelectSection} ${SecGUI}
        ${Else}
            ${UnselectSection} ${SecQuickLaunchIcon}
        ${EndIf}
    ${EndUnless}

    ${IfNot} ${SectionIsSelected} ${SecBoot}
    ${AndIfNot} ${SectionIsSelected} ${SecConsole}
    ${AndIfNot} ${SectionIsSelected} ${SecGUI}
        ${SelectSection} ${SecGUI}
    ${EndIf}

    Pop $R1
    Pop $R0

!macroend

!define ParseCommandLine "!insertmacro ParseCommandLine"

;-----------------------------------------------------------

/*
 * This writes the selections to the registry for future reference
 * add to .onInstSuccess
 */
!macro PreserveInRegistry

    Push $0
    Push $1

    DetailPrint "Saving selections to registry ..."

    SectionGetFlags ${SecBoot} $0
    IntOp $1 $0 & ${SF_SELECTED}
    WriteRegStr HKLM ${UD_UNINSTALL_REG_KEY} "Var::InstallBoot" $1
    ${If} $1 == "0"
        ${RemoveBootFiles}
    ${EndIf}

    SectionGetFlags ${SecConsole} $0
    IntOp $1 $0 & ${SF_SELECTED}
    WriteRegStr HKLM ${UD_UNINSTALL_REG_KEY} "Var::InstallConsole" $1
    ${If} $1 == "0"
        ${RemoveConsoleFiles}
    ${EndIf}

    SectionGetFlags ${SecGUI} $0
    IntOp $1 $0 & ${SF_SELECTED}
    WriteRegStr HKLM ${UD_UNINSTALL_REG_KEY} "Var::InstallGUI" $1
    ${If} $1 == "0"
        ${RemoveGUIFiles}
    ${EndIf}

    SectionGetFlags ${SecHelp} $0
    IntOp $1 $0 & ${SF_SELECTED}
    WriteRegStr HKLM ${UD_UNINSTALL_REG_KEY} "Var::InstallHelp" $1
    ${If} $1 == "0"
        ${RemoveHelpFiles}
    ${EndIf}

    SectionGetFlags ${SecShellHandler} $0
    IntOp $1 $0 & ${SF_SELECTED}
    WriteRegStr HKLM ${UD_UNINSTALL_REG_KEY} "Var::InstallShellHandler" $1
    ${If} $1 == "0"
        ${RemoveShellHandlerFiles}
    ${EndIf}

    SectionGetFlags ${SecUsageTracking} $0
    IntOp $1 $0 & ${SF_SELECTED}
    WriteRegStr HKLM ${UD_UNINSTALL_REG_KEY} "Var::InstallUsageTracking" $1
    ${If} $1 == "0"
        ${RemoveUsageTracking}
    ${EndIf}

    SectionGetFlags ${SecStartMenuIcon} $0
    IntOp $1 $0 & ${SF_SELECTED}
    WriteRegStr HKLM ${UD_UNINSTALL_REG_KEY} "Var::InstallStartMenuIcon" $1
    ${If} $1 == "0"
        ${RemoveStartMenuIcon}
    ${EndIf}

    SectionGetFlags ${SecDesktopIcon} $0
    IntOp $1 $0 & ${SF_SELECTED}
    WriteRegStr HKLM ${UD_UNINSTALL_REG_KEY} "Var::InstallDesktopIcon" $1
    ${If} $1 == "0"
        ${RemoveDesktopIcon}
    ${EndIf}

    SectionGetFlags ${SecQuickLaunchIcon} $0
    IntOp $1 $0 & ${SF_SELECTED}
    WriteRegStr HKLM ${UD_UNINSTALL_REG_KEY} "Var::InstallQuickLaunchIcon" $1
    ${If} $1 == "0"
        ${RemoveQuickLaunchIcon}
    ${EndIf}

    Pop $1
    Pop $0

!macroend

!define PreserveInRegistry "!insertmacro PreserveInRegistry"

;-----------------------------------------------------------

/*
 * This verifies the selections
 * add to .onSelChange
 */
!macro VerifySelections

    ${IfNot} ${SectionIsSelected} ${SecBoot}
    ${AndIfNot} ${SectionIsSelected} ${SecConsole}
    ${AndIfNot} ${SectionIsSelected} ${SecGUI}
        ${SelectSection} ${SecGUI}
    ${EndIf}

    ${IfNot} ${SectionIsSelected} ${SecGUI}
        ${UnselectSection} ${SecShortcuts}
        ${SetSectionFlag} ${SecShortcuts} ${SF_RO}
        ${SetSectionFlag} ${SecStartMenuIcon} ${SF_RO}
        ${SetSectionFlag} ${SecDesktopIcon} ${SF_RO}
        ${SetSectionFlag} ${SecQuickLaunchIcon} ${SF_RO}
    ${ElseIf} ${SectionIsReadOnly} ${SecShortcuts}
        ${ClearSectionFlag} ${SecShortcuts} ${SF_RO}
        ${ClearSectionFlag} ${SecStartMenuIcon} ${SF_RO}
        ${ClearSectionFlag} ${SecDesktopIcon} ${SF_RO}
        ${ClearSectionFlag} ${SecQuickLaunchIcon} ${SF_RO}
    ${EndIf}

    ${IfNot} ${SectionIsSelected} ${SecConsole}
        ${UnselectSection} ${SecShellHandler}
        ${SetSectionFlag} ${SecShellHandler} ${SF_RO}
    ${ElseIf} ${SectionIsReadOnly} ${SecShellHandler}
        ${ClearSectionFlag} ${SecShellHandler} ${SF_RO}
    ${EndIf}

!macroend

!define VerifySelections "!insertmacro VerifySelections"

!endif /* _PRESET_SECTIONS_NSH_ */
