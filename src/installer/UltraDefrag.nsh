/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2019 Dmitri Arkhangelski (dmitriar@gmail.com).
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
* Universal code for both regular and micro edition installers.
*/

!ifndef _ULTRA_DEFRAG_NSH_
!define _ULTRA_DEFRAG_NSH_

/*
* 1. ${DisableX64FSRedirection} is required before
*    all macros except CheckWinVersion and UninstallTheProgram.
* 2. Most macros require the $INSTDIR variable to be set and the
*    plugins directory to be initialized. It is safe to initialize
*    them both in the .onInit function.
*/

!macro LogAndDisplayAbort _Message

    ${If} ${Silent}
        Push $R0
        Push $R1
        Push $R2
        Push $R3

        ${WinVerGetMajor} $R1
        ${WinVerGetMinor} $R2
        ${WinVerGetBuild} $R3

        FileOpen $R0 ${UD_LOG_FILE} w
        ${Unless} ${Errors}
            FileWrite $R0 "$\r$\n"
            FileWrite $R0 "This file contains information to debug installation problems.$\r$\n"
            FileWrite $R0 "$\r$\n"
            FileWrite $R0 "Error Message ..... ${_Message}$\r$\n"
            FileWrite $R0 "$\r$\n"
            FileWrite $R0 "Command Line ...... $CMDLINE$\r$\n"
            FileWrite $R0 "$\r$\n"
            FileWrite $R0 "Installer Path .... $EXEPATH$\r$\n"
            FileWrite $R0 "Installer Type .... $%ULTRADFGARCH%$\r$\n"
            FileWrite $R0 "$\r$\n"
            FileWrite $R0 "Windows Version ... $R1.$R2.$R3$\r$\n"
            FileWrite $R0 "$\r$\n"
            FileWrite $R0 "Install Dir ....... $INSTDIR$\r$\n"
            FileWrite $R0 "Output Dir ........ $OUTDIR$\r$\n"
            FileWrite $R0 "Old Install Dir ... $OldInstallDir$\r$\n"
            FileWrite $R0 "Windows Dir ....... $WINDIR$\r$\n"
            FileWrite $R0 "System Dir ........ $SYSDIR$\r$\n"
            FileWrite $R0 "Plugin Dir ........ $PLUGINSDIR$\r$\n"
            FileWrite $R0 "Temporary Dir ..... $TEMP$\r$\n"
            FileWrite $R0 "$\r$\n"
            FileClose $R0
        ${EndUnless}

        Pop $R3
        Pop $R2
        Pop $R1
        Pop $R0
    ${EndIf}

    MessageBox MB_OK|MB_ICONSTOP "${_Message}" /SD IDOK
!macroend

!define LogAndDisplayAbort "!insertmacro LogAndDisplayAbort"

;-----------------------------------------

!macro CheckAdminRights

    Push $R0

    ; check if the user has administrative rights
    UserInfo::GetAccountType
    Pop $R0

    ${If} $R0 != "Admin"
        ${LogAndDisplayAbort} "Administrative rights are needed to install the program!"
        Abort
    ${EndIf}

    Pop $R0

!macroend

!define CheckAdminRights "!insertmacro CheckAdminRights"

;-----------------------------------------

!macro CheckMutex

    Push $R0

    ; check if UltraDefrag is running
    System::Call 'kernel32::OpenMutex(i 0x100000, b 0, t "Global\ultradefrag_mutex") i .R0'
    ${If} $R0 == 0
        System::Call 'kernel32::OpenMutex(i 0x100000, b 0, t "ultradefrag_mutex") i .R0'
    ${EndIf}
    ${If} $R0 == 0
        System::Call 'kernel32::OpenMutex(i 0x100000, b 0, t "\BaseNamedObjects\ultradefrag_mutex") i .R0'
    ${EndIf}
    ${If} $R0 == 0
        System::Call 'kernel32::OpenMutex(i 0x100000, b 0, t "\Sessions\1\BaseNamedObjects\ultradefrag_mutex") i .R0'
    ${EndIf}
    ${If} $R0 != 0
        System::Call 'kernel32::CloseHandle(i $R0)'
        ${LogAndDisplayAbort} "UltraDefrag is running. Please close it first!"
        Abort
    ${EndIf}

    Pop $R0

!macroend

!define CheckMutex "!insertmacro CheckMutex"

;-----------------------------------------

!macro CheckWinVersion

    ; we only support Windows NT
    ${IfNot} ${IsNT}
        ${LogAndDisplayAbort} \
            "This program cannot be used on Windows 95, 98 and Me!"
        Abort
    ${EndIf}

    ; we only support Windows XP and above
    ${IfNot} ${AtLeastWinXP}
        ${LogAndDisplayAbort} \
            "This program cannot be used on Windows versions below XP!$\n \
            Download UltraDefrag v6 for Windows NT 4.0 and Windows 2000."
        Abort
    ${EndIf}
    
    ; binaries built with Windows SDK 7.1 require at least Windows XP SP2
    ${If} ${IsWinXP}
        ${IfNot} ${AtLeastServicePack} 2
            ${LogAndDisplayAbort} \
                "This program requires at least Windows XP Service Pack 2 to be installed!"
            Abort
        ${EndIf}
    ${EndIf}

    /* this idea was suggested by bender647 at users.sourceforge.net */
    Push $R0
    ClearErrors
    ; ReadEnvStr $R0 "PROCESSOR_ARCHITECTURE"
    ; On 64-bit systems it always returns 'x86' because the installer
    ; is a 32-bit application and runs on a virtual machine :(((

    ; read the PROCESSOR_ARCHITECTURE variable from registry
    ReadRegStr $R0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" \
    "PROCESSOR_ARCHITECTURE"

    ${Unless} ${Errors}
        ${If} $R0 == "x86"
        ${AndIf} "$%ULTRADFGARCH%" != "i386"
            ${LogAndDisplayAbort} \
                "This installer cannot be used on 32-bit Windows!$\n \
                Download the i386 version from https://ultradefrag.net/"
            Pop $R0
            Abort
        ${EndIf}
        ${If} $R0 == "amd64"
        ${AndIf} "$%ULTRADFGARCH%" != "amd64"
            ${LogAndDisplayAbort} \
                "This installer cannot be used on x64 versions of Windows!$\n \
                Download the amd64 version from https://ultradefrag.net/"
            Pop $R0
            Abort
        ${EndIf}
        ${If} $R0 == "ia64"
        ${AndIf} "$%ULTRADFGARCH%" != "ia64"
            ${LogAndDisplayAbort} \
                "This installer cannot be used on IA-64 versions of Windows!$\n \
                Download the ia64 version from https://ultradefrag.net/"
            Pop $R0
            Abort
        ${EndIf}
    ${EndUnless}
    Pop $R0

!macroend

!define CheckWinVersion "!insertmacro CheckWinVersion"

;-----------------------------------------

/**
 * This procedure returns system DPI in $R0.
 * Possible return values: 96, 120, 144, 192.
 * All the intermediate values get rounded up,
 * all the values above 192 get rounded down.
 */
!macro GetSystemDPI

    Push $0
    Push $1
    
    System::Call USER32::GetDpiForSystem()i.r0
    ${If} $0 U<= 0
        System::Call USER32::GetDC(i0)i.r1 
        System::Call GDI32::GetDeviceCaps(ir1,i88)i.r0 
        System::Call USER32::ReleaseDC(i0,ir1)
    ${EndIf}
    
    ${If} $0 > 144
        StrCpy $R0 192
    ${ElseIf} $0 > 120
        StrCpy $R0 144
    ${ElseIf} $0 > 96
        StrCpy $R0 120
    ${Else}
        StrCpy $R0 96
    ${EndIf}
    
    Pop $1
    Pop $0
    
!macroend

!define GetSystemDPI "!insertmacro GetSystemDPI"

;-----------------------------------------

!macro SetPageBitmap _Page

    Push $R0

    ${EnableX64FSRedirection}
    ${GetSystemDPI}
    ${NSD_SetImage} $mui.${_Page}.Image $PLUGINSDIR\WelcomePageBitmap$R0.bmp $mui.${_Page}.Image.Bitmap
    ${DisableX64FSRedirection}
    
    Pop $R0

!macroend

!define SetPageBitmap "!insertmacro SetPageBitmap"

;-----------------------------------------

/**
 * This procedure validates destination folder and is
 * only used by the directory page verification callback.
 * Only empty folders or folders containing an existing
 * UltraDefrag installation are valid.
 */
!macro CheckDestFolder

    StrCpy $ValidDestDir "1"

    ; if $INSTDIR is an ultradefrag directory, let us install there
    IfFileExists "$INSTDIR\lua5.1a_gui.exe" PathGood

    ; if $INSTDIR is not empty, don't let us install there
    Push $R1
    Push $R2

    FindFirst $R1 $R2 "$INSTDIR\*"
    ${If} $R1 != ""
        ${If} $R2 != ""
            ${Do}
                ${If} $R2 != "."
                ${AndIf} $R2 != ".."
                    ${ExitDo}
                ${EndIf}
                FindNext $R1 $R2
            ${Loop}
        ${EndIf}
        FindClose $R1
    ${EndIf}

    ${If} $R2 != ""
        StrCpy $ValidDestDir "0"
    ${EndIf}

    Pop $R2
    Pop $R1

PathGood:

!macroend

!define CheckDestFolder "!insertmacro CheckDestFolder"

;-----------------------------------------

/**
 * This procedure installs all mandatory files and
 * upgrades already existing configuration files
 * if they are in an obsolete format.
 */
!macro InstallCoreFiles

    ${DisableX64FSRedirection}

    ; validate destination folder for silent installations here,
    ; since the directory page is not displayed in the silent mode
    ${If} ${Silent}
        ${CheckDestFolder}
        ${If} $ValidDestDir == "0"
            ${LogAndDisplayAbort} "Destination folder is invalid!"
            Abort
        ${EndIf}
    ${EndIf}

    ; relocate the installation
    IfFileExists "$OldInstallDir\*.*" 0 SkipMove
    StrCmp "$INSTDIR" "$OldInstallDir" SkipMove

    DetailPrint "Relocating the installation..."
    CreateDirectory "$INSTDIR"
    ClearErrors
    CopyFiles /SILENT "$OldInstallDir\*" "$INSTDIR"
    ${If} ${Errors}
        ${LogAndDisplayAbort} "Cannot relocate the installation!"
        Abort
    ${EndIf}

    SetDetailsPrint textonly
    RMDir /r "$OldInstallDir"
    SetDetailsPrint both

SkipMove:
    DetailPrint "Removing old executable files..."
    Delete "$INSTDIR\*.exe"
    Delete "$INSTDIR\*.dll"

    DetailPrint "Installing core files..."
    SetOutPath "$SYSDIR"
    File "lua5.1a.dll"
    File "zenwinx.dll"
    File "udefrag.dll"
    File /oname=hibernate4win.exe "hibernate.exe"
    File "udefrag-dbg.exe"

    SetOutPath "$INSTDIR"
    File "${ROOTDIR}\src\HISTORY.TXT"
    File "${ROOTDIR}\src\LICENSE.TXT"
    File "README.TXT"

    File "lua5.1a.exe"
    File "lua5.1a_gui.exe"

    SetOutPath "$INSTDIR\scripts"
    File "${ROOTDIR}\src\scripts\udreportcnv.lua"
    File "${ROOTDIR}\src\scripts\udsorting.js"
    File "${ROOTDIR}\src\scripts\upgrade-options.lua"

    DetailPrint "Configuration files upgrade..."
    ; ensure that target directory exists
    CreateDirectory "$INSTDIR\conf"
    ${If} ${Silent}
        ExecWait '"$INSTDIR\lua5.1a_gui.exe" -s "$INSTDIR\scripts\upgrade-options.lua" "$INSTDIR"'
    ${Else}
        ExecWait '"$INSTDIR\lua5.1a_gui.exe" "$INSTDIR\scripts\upgrade-options.lua" "$INSTDIR"'
    ${EndIf}

    ; install default CSS for file fragmentation reports
    File "${ROOTDIR}\src\scripts\udreport.css"

    DetailPrint "Registering .luar file extension..."
    WriteRegStr HKCR ".luar" "" "LuaReport"
    WriteRegStr HKCR "LuaReport" "" "Lua Report"
    WriteRegStr HKCR "LuaReport\DefaultIcon" "" "$INSTDIR\lua5.1a_gui.exe,1"
    WriteRegStr HKCR "LuaReport\shell" "" "view"
    WriteRegStr HKCR "LuaReport\shell\view" "" "View report"
    WriteRegStr HKCR "LuaReport\shell\view\command" "" \
        "$\"$INSTDIR\lua5.1a_gui.exe$\" $\"$INSTDIR\scripts\udreportcnv.lua$\" $\"%1$\" -v"

    ${EnableX64FSRedirection}

!macroend

!define InstallCoreFiles "!insertmacro InstallCoreFiles"

;-----------------------------------------

!macro RemoveCoreFiles

    ${DisableX64FSRedirection}

    DetailPrint "Removing core files..."
    Delete "$INSTDIR\HISTORY.TXT"
    Delete "$INSTDIR\LICENSE.TXT"
    Delete "$INSTDIR\README.TXT"
    Delete "$INSTDIR\lua5.1a.exe"
    Delete "$INSTDIR\lua5.1a_gui.exe"
    RMDir /r "$INSTDIR\scripts"
    RMDir /r "$INSTDIR\conf"

    Delete "$SYSDIR\zenwinx.dll"
    Delete "$SYSDIR\udefrag.dll"
    Delete "$SYSDIR\lua5.1a.dll"
    Delete "$SYSDIR\hibernate4win.exe"
    Delete "$SYSDIR\udefrag-dbg.exe"

    DetailPrint "Deregistering .luar file extension..."
    DeleteRegKey HKCR "LuaReport"
    DeleteRegKey HKCR ".luar"

    ${EnableX64FSRedirection}

!macroend

!define RemoveCoreFiles "!insertmacro RemoveCoreFiles"

;-----------------------------------------

!macro InstallBootFiles

    ${DisableX64FSRedirection}

    ${If} ${FileExists} "$SYSDIR\defrag_native.exe"
        DetailPrint "Removing old boot time interface..."
        ClearErrors
        Delete "$SYSDIR\defrag_native.exe"
        ${If} ${Errors}
            /*
            * It's not safe to leave it as is, because some
            * older versions of it may cause BSOD in case of
            * inconsistency between them and udefrag/zenwinx
            * libraries they depend on.
            */
            ${LogAndDisplayAbort} "Cannot update $SYSDIR\defrag_native.exe file!"
            ; turn the boot time interface off
            ExecWait '"$SYSDIR\bootexctrl.exe" /u /s defrag_native'
            ; try to remove it once again, just for safety
            ${EnableX64FSRedirection}
            SetOutPath $PLUGINSDIR
            File "bootexctrl.exe"
            ExecWait '"$PLUGINSDIR\bootexctrl.exe" /u /s defrag_native'
            ; abort the installation
            Abort
        ${EndIf}
    ${EndIf}

    DetailPrint "Installing boot time interface..."
    SetOutPath "$INSTDIR\man"
    File "${ROOTDIR}\doc\man\*.*"

    SetOutPath "$SYSDIR"
    File "${ROOTDIR}\src\installer\boot-config.cmd"
    File "${ROOTDIR}\src\installer\boot-off.cmd"
    File "${ROOTDIR}\src\installer\boot-on.cmd"
    File "bootexctrl.exe"
    File "defrag_native.exe"

    ${Unless} ${FileExists} "$SYSDIR\ud-boot-time.cmd"
        File "${ROOTDIR}\src\installer\ud-boot-time.cmd"
        File "${ROOTDIR}\src\installer\ud-boot-time.ini"
    ${Else}
        ; the script of v5.0 is not compatible with previous
        ; versions because of filter syntax changes
        ${Unless} ${FileExists} "$SYSDIR\ud-boot-time.ini"
            Rename "$SYSDIR\ud-boot-time.cmd" "$SYSDIR\ud-boot-time.cmd.old"
            File "${ROOTDIR}\src\installer\ud-boot-time.cmd"
            File "${ROOTDIR}\src\installer\ud-boot-time.ini"
        ${EndUnless}
    ${EndUnless}

    ${EnableX64FSRedirection}

!macroend

!define InstallBootFiles "!insertmacro InstallBootFiles"

;-----------------------------------------

!macro RemoveBootFiles

    ${DisableX64FSRedirection}

    DetailPrint "Removing boot time interface..."
    RMDir /r "$INSTDIR\man"

    ExecWait '"$SYSDIR\bootexctrl.exe" /u /s defrag_native'

    Delete "$SYSDIR\boot-config.cmd"
    Delete "$SYSDIR\boot-off.cmd"
    Delete "$SYSDIR\boot-on.cmd"
    Delete "$SYSDIR\bootexctrl.exe"
    Delete "$SYSDIR\defrag_native.exe"
    Delete "$SYSDIR\ud-boot-time.cmd"
    Delete "$SYSDIR\ud-boot-time.cmd.old"
    Delete "$SYSDIR\ud-boot-time.ini"

    ${EnableX64FSRedirection}

!macroend

!define RemoveBootFiles "!insertmacro RemoveBootFiles"

;-----------------------------------------

!macro InstallConsoleFiles

    ${DisableX64FSRedirection}

    DetailPrint "Installing console interface..."
    SetOutPath "$SYSDIR"
    File "udefrag.exe"

    ${EnableX64FSRedirection}

!macroend

!define InstallConsoleFiles "!insertmacro InstallConsoleFiles"

;-----------------------------------------

!macro RemoveConsoleFiles

    ${DisableX64FSRedirection}

    DetailPrint "Removing console interface..."
    Delete "$SYSDIR\udefrag.exe"

    ${EnableX64FSRedirection}

!macroend

!define RemoveConsoleFiles "!insertmacro RemoveConsoleFiles"

;-----------------------------------------

!macro InstallGUIFiles

    ${DisableX64FSRedirection}

    DetailPrint "Installing graphical interface..."
    SetOutPath "$INSTDIR"
    File /nonfatal /r /x *.pot /x *.header /x *.svn "${ROOTDIR}\src\wxgui\locale"

    SetOutPath "$INSTDIR\po"
    File /nonfatal "${ROOTDIR}\src\tools\transifex\translations\ultradefrag.main\*.po"
    File /nonfatal "${ROOTDIR}\src\wxgui\locale\*.pot"

    SetOutPath "$INSTDIR"
    Delete "$INSTDIR\ultradefrag.exe"
    File "ultradefrag.exe"

    DetailPrint "Fragmentation reports translation update..."
    ExecWait '"$INSTDIR\ultradefrag.exe" --setup'

    Push $R0
    Push $0

    DetailPrint "Registering file extensions..."
    ; Without $SYSDIR because x64 systems apply registry redirection for HKCR before writing.
    ; Whenever we are using $SYSDIR Windows converts it to C:\WINDOWS\SysWow64.

    ClearErrors
    ReadRegStr $R0 HKCR ".lua" ""
    ${If} ${Errors}
        WriteRegStr HKCR ".lua" "" "Lua.Script"
        WriteRegStr HKCR "Lua.Script" "" "Lua Script File"
        WriteRegStr HKCR "Lua.Script\shell\Edit" "" "Edit Script"
        WriteRegStr HKCR "Lua.Script\shell\Edit\command" "" "notepad.exe %1"

        WriteRegStr HKLM ${UD_UNINSTALL_REG_KEY} "Registered.lua" "1"
    ${Else}
        StrCpy $0 $R0
        ClearErrors
        ReadRegStr $R0 HKCR "$0\shell\Edit" ""
        ${If} ${Errors}
            WriteRegStr HKCR "$0\shell\Edit" "" "Edit Script"
            WriteRegStr HKCR "$0\shell\Edit\command" "" "notepad.exe %1"

            WriteRegStr HKLM ${UD_UNINSTALL_REG_KEY} "Registered.lua.edit" "1"
        ${EndIf}
    ${EndIf}

    Pop $0
    Pop $R0

    ${EnableX64FSRedirection}

!macroend

!define InstallGUIFiles "!insertmacro InstallGUIFiles"

;-----------------------------------------

!macro RemoveGUIFiles

    ${DisableX64FSRedirection}

    DetailPrint "Removing graphical interface..."
    RMDir /r "$INSTDIR\locale"
    RMDir /r "$INSTDIR\po"

    Delete "$INSTDIR\ultradefrag.exe"

    Push $R0

    DetailPrint "Deregistering file extensions..."
    ClearErrors
    ReadRegStr $R0 HKLM ${UD_UNINSTALL_REG_KEY} "Registered.lua"
    ${Unless} ${Errors}
        ${If} $R0 == "1"
            DeleteRegKey HKCR "Lua.Script"
            DeleteRegKey HKCR ".lua"
            DeleteRegValue HKLM ${UD_UNINSTALL_REG_KEY} "Registered.lua"
        ${EndIf}
    ${EndUnless}

    ClearErrors
    ReadRegStr $R0 HKLM ${UD_UNINSTALL_REG_KEY} "Registered.lua.edit"
    ${Unless} ${Errors}
        ${If} $R0 == "1"
            ClearErrors
            ReadRegStr $R0 HKCR ".lua" ""
            ${Unless} ${Errors}
                DeleteRegKey HKCR "$R0\shell\Edit"
                DeleteRegValue HKLM ${UD_UNINSTALL_REG_KEY} "Registered.lua.edit"
            ${EndUnless}
        ${EndIf}
    ${EndUnless}

    Pop $R0

    ${EnableX64FSRedirection}

!macroend

!define RemoveGUIFiles "!insertmacro RemoveGUIFiles"

;-----------------------------------------

!macro InstallHelpFiles

    ${DisableX64FSRedirection}

    DetailPrint "Installing documentation..."
    RMDir /r "$INSTDIR\handbook"

    SetOutPath "$INSTDIR\handbook"
    File "${ROOTDIR}\doc\handbook\doxy-doc\html\*.*"

    ${EnableX64FSRedirection}

!macroend

!define InstallHelpFiles "!insertmacro InstallHelpFiles"

;-----------------------------------------

!macro RemoveHelpFiles

    ${DisableX64FSRedirection}

    DetailPrint "Removing documentation..."
    RMDir /r "$INSTDIR\handbook"

    ${EnableX64FSRedirection}

!macroend

!define RemoveHelpFiles "!insertmacro RemoveHelpFiles"

;-----------------------------------------

!macro InstallShellHandlerFiles

    ; get rid of old context menu handler to make
    ; sure that it won't interfere with the new one
    ${RemoveShellHandlerFiles}
    
    ${DisableX64FSRedirection}

    DetailPrint "Installing context menu handler..."
    SetOutPath "$INSTDIR\icons"
    File "${ROOTDIR}\src\installer\shellex.ico"
    File "${ROOTDIR}\src\installer\shellex-folder.ico"

    Push $0
    Push $1
    Push $R0
    Push $R1

    StrCpy $0 "$INSTDIR\icons\shellex.ico"
    StrCpy $1 "$INSTDIR\icons\shellex-folder.ico"

    ${If} ${AtLeastWin7}
        WriteRegStr HKCR "Drive\shell\udefrag.W7menu" "MUIVerb"                "&UltraDefrag"
        WriteRegStr HKCR "Drive\shell\udefrag.W7menu" "ExtendedSubCommandsKey" "Drive\udefragW7menu"
        WriteRegStr HKCR "Drive\shell\udefrag.W7menu" "Icon" $0

        StrCpy $R0 "&Analyze"
        StrCpy $R1 "$\"$SYSDIR\udefrag.exe$\" --shellex --folder -a -v $\"%1$\""
        WriteRegStr HKCR "Drive\udefragW7menu\shell\udefrag-drive-analyze"            ""     $R0
        WriteRegStr HKCR "Drive\udefragW7menu\shell\udefrag-drive-analyze\command"    ""     $R1

        StrCpy $R0 "&Defragment"
        StrCpy $R1 "$\"$SYSDIR\udefrag.exe$\" --shellex --folder $\"%1$\""
        WriteRegStr HKCR "Drive\udefragW7menu\shell\udefrag-drive-defragment"         ""     $R0
        WriteRegStr HKCR "Drive\udefragW7menu\shell\udefrag-drive-defragment\command" ""     $R1

        StrCpy $R0 "Perform &full optimization"
        StrCpy $R1 "$\"$SYSDIR\udefrag.exe$\" --shellex --folder -o -v $\"%1$\""
        WriteRegStr HKCR "Drive\udefragW7menu\shell\udefrag-drive-full-optimization"          ""     $R0
        WriteRegStr HKCR "Drive\udefragW7menu\shell\udefrag-drive-full-optimization\command"  ""     $R1

        StrCpy $R0 "Perform &quick optimization"
        StrCpy $R1 "$\"$SYSDIR\udefrag.exe$\" --shellex --folder -q -v $\"%1$\""
        WriteRegStr HKCR "Drive\udefragW7menu\shell\udefrag-drive-quick-optimization"         ""     $R0
        WriteRegStr HKCR "Drive\udefragW7menu\shell\udefrag-drive-quick-optimization\command" ""     $R1

        StrCpy $R0 "Defragment &root folder itself"
        StrCpy $R1 "$\"$SYSDIR\udefrag.exe$\" --shellex --folder-itself $\"%1$\""
        WriteRegStr HKCR "Drive\udefragW7menu\shell\udefrag-folder"                  ""     $R0
        WriteRegStr HKCR "Drive\udefragW7menu\shell\udefrag-folder"                  "Icon" $1
        WriteRegStr HKCR "Drive\udefragW7menu\shell\udefrag-folder\command"          ""     $R1

        WriteRegStr HKCR "Directory\shell\udefrag.W7menu" "MUIVerb"                "&UltraDefrag"
        WriteRegStr HKCR "Directory\shell\udefrag.W7menu" "ExtendedSubCommandsKey" "Directory\udefragW7menu"
        WriteRegStr HKCR "Directory\shell\udefrag.W7menu" "Icon" $0

        StrCpy $R0 "&Defragment"
        StrCpy $R1 "$\"$SYSDIR\udefrag.exe$\" --shellex --folder $\"%1$\""
        WriteRegStr HKCR "Directory\udefragW7menu\shell\udefrag"                ""     $R0
        WriteRegStr HKCR "Directory\udefragW7menu\shell\udefrag\command"        ""     $R1

        StrCpy $R0 "&Defragment folder itself"
        StrCpy $R1 "$\"$SYSDIR\udefrag.exe$\" --shellex --folder-itself $\"%1$\""
        WriteRegStr HKCR "Directory\udefragW7menu\shell\udefrag-folder"         ""     $R0
        WriteRegStr HKCR "Directory\udefragW7menu\shell\udefrag-folder"         "Icon" $1
        WriteRegStr HKCR "Directory\udefragW7menu\shell\udefrag-folder\command" ""     $R1

        StrCpy $R0 "&Defragment with UltraDefrag"
        StrCpy $R1 "$\"$SYSDIR\udefrag.exe$\" --shellex $\"%1$\""
        WriteRegStr HKCR "*\shell\udefrag"         ""     $R0
        WriteRegStr HKCR "*\shell\udefrag"         "Icon" $0
        WriteRegStr HKCR "*\shell\udefrag\command" ""     $R1
    ${Else}
        StrCpy $R0 "&Analyze with UltraDefrag"
        StrCpy $R1 "$\"$SYSDIR\udefrag.exe$\" --shellex --folder -a -v $\"%1$\""
        WriteRegStr HKCR "Drive\shell\udefrag-drive-analyze"            ""     $R0
        WriteRegStr HKCR "Drive\shell\udefrag-drive-analyze\command"    ""     $R1

        StrCpy $R0 "&Defragment with UltraDefrag"
        StrCpy $R1 "$\"$SYSDIR\udefrag.exe$\" --shellex --folder $\"%1$\""
        WriteRegStr HKCR "Drive\shell\udefrag-drive-defragment"         ""     $R0
        WriteRegStr HKCR "Drive\shell\udefrag-drive-defragment\command" ""     $R1

        StrCpy $R0 "Perform &full optimization with UltraDefrag"
        StrCpy $R1 "$\"$SYSDIR\udefrag.exe$\" --shellex --folder -o -v $\"%1$\""
        WriteRegStr HKCR "Drive\shell\udefrag-drive-full-optimization"          ""     $R0
        WriteRegStr HKCR "Drive\shell\udefrag-drive-full-optimization\command"  ""     $R1

        StrCpy $R0 "Perform &quick optimization with UltraDefrag"
        StrCpy $R1 "$\"$SYSDIR\udefrag.exe$\" --shellex --folder -q -v $\"%1$\""
        WriteRegStr HKCR "Drive\shell\udefrag-drive-quick-optimization"         ""     $R0
        WriteRegStr HKCR "Drive\shell\udefrag-drive-quick-optimization\command" ""     $R1

        StrCpy $R0 "Defragment &root folder itself with UltraDefrag"
        StrCpy $R1 "$\"$SYSDIR\udefrag.exe$\" --shellex --folder-itself $\"%1$\""
        WriteRegStr HKCR "Drive\shell\udefrag-folder"                  ""     $R0
        WriteRegStr HKCR "Drive\shell\udefrag-folder\command"          ""     $R1

        StrCpy $R0 "&Defragment with UltraDefrag"
        StrCpy $R1 "$\"$SYSDIR\udefrag.exe$\" --shellex --folder $\"%1$\""
        WriteRegStr HKCR "Directory\shell\udefrag"                ""     $R0
        WriteRegStr HKCR "Directory\shell\udefrag\command"        ""     $R1

        StrCpy $R0 "&Defragment folder itself with UltraDefrag"
        StrCpy $R1 "$\"$SYSDIR\udefrag.exe$\" --shellex --folder-itself $\"%1$\""
        WriteRegStr HKCR "Directory\shell\udefrag-folder"         ""     $R0
        WriteRegStr HKCR "Directory\shell\udefrag-folder\command" ""     $R1

        StrCpy $R0 "&Defragment with UltraDefrag"
        StrCpy $R1 "$\"$SYSDIR\udefrag.exe$\" --shellex $\"%1$\""
        WriteRegStr HKCR "*\shell\udefrag"         ""     $R0
        WriteRegStr HKCR "*\shell\udefrag\command" ""     $R1
    ${EndIf}

    Pop $R1
    Pop $R0
    Pop $1
    Pop $0

    ${EnableX64FSRedirection}

!macroend

!define InstallShellHandlerFiles "!insertmacro InstallShellHandlerFiles"

;-----------------------------------------

!macro RemoveShellHandlerFiles

    ${DisableX64FSRedirection}

    DetailPrint "Removing context menu handler..."
    Delete "$INSTDIR\icons\shellex.ico"
    Delete "$INSTDIR\icons\shellex-folder.ico"
    RMDir "$INSTDIR\icons"

    DeleteRegKey HKCR "Drive\shell\udefrag.W7menu"
    DeleteRegKey HKCR "Drive\udefragW7menu"
    DeleteRegKey HKCR "Directory\shell\udefrag.W7menu"
    DeleteRegKey HKCR "Directory\udefragW7menu"

    DeleteRegKey HKCR "Drive\shell\udefrag-drive-analyze"
    DeleteRegKey HKCR "Drive\shell\udefrag-drive-defragment"
    DeleteRegKey HKCR "Drive\shell\udefrag-drive-full-optimization"
    DeleteRegKey HKCR "Drive\shell\udefrag-drive-quick-optimization"
    DeleteRegKey HKCR "Drive\shell\udefrag-folder"
    DeleteRegKey HKCR "Directory\shell\udefrag"
    DeleteRegKey HKCR "Directory\shell\udefrag-folder"
    DeleteRegKey HKCR "*\shell\udefrag"

    ${EnableX64FSRedirection}

!macroend

!define RemoveShellHandlerFiles "!insertmacro RemoveShellHandlerFiles"

;-----------------------------------------

!macro InstallUsageTracking

    Push $0

    DetailPrint "Disabling usage tracking..."
    StrCpy $0 "UD_DISABLE_USAGE_TRACKING"
    WriteRegStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" $0 "1"
    WriteRegStr HKCU "Environment" $0 "1"

    ; "Export" our change
    SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000

    Pop $0

!macroend

!define InstallUsageTracking "!insertmacro InstallUsageTracking"

;-----------------------------------------

!macro RemoveUsageTracking

    Push $0

    StrCpy $0 "UD_DISABLE_USAGE_TRACKING"
    DeleteRegValue HKCU "Environment" $0
    DeleteRegValue HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" $0
    DeleteRegValue HKLM "SYSTEM\ControlSet001\Control\Session Manager\Environment" $0
    DeleteRegValue HKLM "SYSTEM\ControlSet002\Control\Session Manager\Environment" $0

    ; "Export" our change
    SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000

    Pop $0

!macroend

!define RemoveUsageTracking "!insertmacro RemoveUsageTracking"

;-----------------------------------------

!macro InstallStartMenuIcon

    ${DisableX64FSRedirection}

    DetailPrint "Installing start menu icon..."
    SetShellVarContext all
    SetOutPath $INSTDIR

    ; install a single shortcut to the Start menu,
    ; because all important information can be easily
    ; accessed from the GUI
    CreateShortCut "$SMPROGRAMS\UltraDefrag.lnk" \
        "$INSTDIR\ultradefrag.exe"

    ${EnableX64FSRedirection}

!macroend

!define InstallStartMenuIcon "!insertmacro InstallStartMenuIcon"

;-----------------------------------------

!macro RemoveStartMenuIcon

    ${DisableX64FSRedirection}

    DetailPrint "Removing start menu icon..."
    SetShellVarContext all
    Delete "$SMPROGRAMS\UltraDefrag.lnk"

    ${EnableX64FSRedirection}

!macroend

!define RemoveStartMenuIcon "!insertmacro RemoveStartMenuIcon"

;-----------------------------------------

!macro InstallDesktopIcon

    ${DisableX64FSRedirection}

    DetailPrint "Installing desktop icon..."
    SetShellVarContext all
    SetOutPath $INSTDIR
    CreateShortCut "$DESKTOP\UltraDefrag.lnk" \
        "$INSTDIR\ultradefrag.exe"

    ${EnableX64FSRedirection}

!macroend

!define InstallDesktopIcon "!insertmacro InstallDesktopIcon"

;-----------------------------------------

!macro RemoveDesktopIcon

    ${DisableX64FSRedirection}

    DetailPrint "Removing desktop icon..."
    SetShellVarContext all
    Delete "$DESKTOP\UltraDefrag.lnk"

    ${EnableX64FSRedirection}

!macroend

!define RemoveDesktopIcon "!insertmacro RemoveDesktopIcon"

;-----------------------------------------

!macro InstallQuickLaunchIcon

    ${DisableX64FSRedirection}

    DetailPrint "Installing quick launch icon..."
    SetShellVarContext all
    SetOutPath $INSTDIR
    CreateShortCut "$QUICKLAUNCH\UltraDefrag.lnk" \
        "$INSTDIR\ultradefrag.exe"

    ${EnableX64FSRedirection}

!macroend

!define InstallQuickLaunchIcon "!insertmacro InstallQuickLaunchIcon"

;-----------------------------------------

!macro RemoveQuickLaunchIcon

    ${DisableX64FSRedirection}

    DetailPrint "Removing quick launch icon..."
    SetShellVarContext all
    Delete "$QUICKLAUNCH\UltraDefrag.lnk"

    ${EnableX64FSRedirection}

!macroend

!define RemoveQuickLaunchIcon "!insertmacro RemoveQuickLaunchIcon"

;-----------------------------------------

!macro RemoveObsoleteFiles
    DetailPrint "Removing obsolete files and settings..."

    ; remove files of previous installations
    DeleteRegKey HKLM "SYSTEM\UltraDefrag"
    DeleteRegKey HKLM "SYSTEM\CurrentControlSet\Control\UltraDefrag"
    DeleteRegKey HKLM "SYSTEM\ControlSet001\Control\UltraDefrag"
    DeleteRegKey HKLM "SYSTEM\ControlSet002\Control\UltraDefrag"
    DeleteRegKey HKLM "SYSTEM\ControlSet003\Control\UltraDefrag"
    SetRegView 64
    DeleteRegKey HKLM "Software\UltraDefrag"
    SetRegView 32
    DeleteRegKey HKLM "Software\UltraDefrag"

    DeleteRegKey HKCR "Drive\shell\udefrag"
    DeleteRegKey HKCR "Drive\shell\udefrag-drive-optimize"
    DeleteRegKey HKCR "Drive\shell\udefrag-drive-qoptimize"
    DeleteRegKey HKCR "Folder\shell\udefrag"
    DeleteRegKey HKCR "Folder\shell\udefrag-folder"
    DeleteRegKey HKCR "Folder\shell\udefrag.W7menu"
    DeleteRegKey HKCR "Folder\udefragW7menu"
    DeleteRegKey HKCR "*\shell\udefrag.W7menu"
    DeleteRegKey HKCR "*\udefragW7menu"
    
    RMDir /r "$SYSDIR\UltraDefrag"
    Delete "$SYSDIR\udefrag-gui-dbg.cmd"
    Delete "$SYSDIR\udefrag-gui.exe"
    Delete "$SYSDIR\udefrag-gui.cmd"
    Delete "$SYSDIR\ultradefrag.exe"
    Delete "$SYSDIR\udefrag-gui-config.exe"
    Delete "$SYSDIR\udefrag-scheduler.exe"
    Delete "$SYSDIR\ud-config.cmd"
    Delete "$SYSDIR\ud-help.cmd"
    Delete "$SYSDIR\udefrag-kernel.dll"
    Delete "$SYSDIR\lua5.1a.exe"
    Delete "$SYSDIR\lua5.1a_gui.exe"
    Delete "$SYSDIR\udctxhandler.cmd"
    Delete "$SYSDIR\udctxhandler.vbs"
    Delete "$SYSDIR\wgx.dll"

    RMDir /r "$INSTDIR\doc"
    RMDir /r "$INSTDIR\i18n"
    RMDir /r "$INSTDIR\options"
    RMDir /r "$INSTDIR\portable_$%ULTRADFGARCH%_package"
    RMDir /r "$INSTDIR\presets"

    Delete "$INSTDIR\scripts\udctxhandler.lua"
    Delete "$INSTDIR\scripts\upgrade-guiopts.lua"
    Delete "$INSTDIR\scripts\upgrade-rptopts.lua"
    Delete "$INSTDIR\scripts\udreport.css.old"

    Delete "$INSTDIR\options.lua"
    Delete "$INSTDIR\options.lua.old"

    Delete "$INSTDIR\dfrg.exe"
    Delete "$INSTDIR\CREDITS.TXT"
    Delete "$INSTDIR\INSTALL.TXT"
    Delete "$INSTDIR\FAQ.TXT"
    Delete "$INSTDIR\UltraDefragScheduler.NET.exe"
    Delete "$INSTDIR\boot_on.cmd"
    Delete "$INSTDIR\boot_off.cmd"
    Delete "$INSTDIR\ud_i18n.dll"
    Delete "$INSTDIR\wgx.dll"
    Delete "$INSTDIR\lua5.1a.dll"
    Delete "$INSTDIR\repair-drives.cmd"

    Delete "$INSTDIR\udefrag-scheduler.exe"
    Delete "$INSTDIR\udefrag-gui-config.exe"
    Delete "$INSTDIR\LanguageSelector.exe"
    Delete "$INSTDIR\lang.ini"
    Delete "$INSTDIR\wxultradefrag.exe"

    Delete "$INSTDIR\shellex.ico"
    Delete "$INSTDIR\shellex-folder.ico"

    Delete "$INSTDIR\crash-info.ini"
    Delete "$INSTDIR\crash-info.log"
    
    ; remove outdated *.lng files, but keep installed reports.lng
    Rename "$INSTDIR\reports.lng" "$INSTDIR\reports.tmp"
    Delete "$INSTDIR\*.lng"
    Rename "$INSTDIR\reports.tmp" "$INSTDIR\reports.lng"

    ; remove empty translations
    Delete "$INSTDIR\po\ach.po"
    Delete "$INSTDIR\po\ar_EG.po"
    Delete "$INSTDIR\po\ar_SA.po"
    Delete "$INSTDIR\po\eu.po"
    Delete "$INSTDIR\po\eu_ES.po"
    Delete "$INSTDIR\po\si_LK.po"
    Delete "$INSTDIR\po\szl.po"
    RMDir /r "$INSTDIR\locale\ach"
    RMDir /r "$INSTDIR\locale\ar_EG"
    RMDir /r "$INSTDIR\locale\ar_SA"
    RMDir /r "$INSTDIR\locale\eu"
    RMDir /r "$INSTDIR\locale\eu_ES"
    RMDir /r "$INSTDIR\locale\si_LK"
    RMDir /r "$INSTDIR\locale\szl"
    
    RMDir /r "$INSTDIR\tmp\data"
    
    ; remove obsolete shortcuts
    SetShellVarContext all
    RMDir /r "$SMPROGRAMS\DASoft"
    RMDir /r "$SMPROGRAMS\UltraDefrag"

    Delete "$SYSDIR\Drivers\ultradfg.sys"
    DeleteRegKey HKLM "SYSTEM\CurrentControlSet\Services\ultradfg"
    DeleteRegKey HKLM "SYSTEM\ControlSet001\Services\ultradfg"
    DeleteRegKey HKLM "SYSTEM\ControlSet002\Services\ultradfg"
    DeleteRegKey HKLM "SYSTEM\ControlSet003\Services\ultradfg"
    DeleteRegKey HKLM "SYSTEM\CurrentControlSet\Enum\Root\LEGACY_ULTRADFG"
    DeleteRegKey HKLM "SYSTEM\ControlSet001\Enum\Root\LEGACY_ULTRADFG"
    DeleteRegKey HKLM "SYSTEM\ControlSet002\Enum\Root\LEGACY_ULTRADFG"
    DeleteRegKey HKLM "SYSTEM\ControlSet003\Enum\Root\LEGACY_ULTRADFG"

    ; remove obsolete file associations
    ClearErrors
    ReadRegStr $R0 HKLM ${UD_UNINSTALL_REG_KEY} "Registered.lng"
    ${Unless} ${Errors}
        ${If} $R0 == "1"
            DeleteRegKey HKCR "LanguagePack"
            DeleteRegKey HKCR ".lng"
            DeleteRegValue HKLM ${UD_UNINSTALL_REG_KEY} "Registered.lng"
        ${EndIf}
    ${EndUnless}

!macroend

!define RemoveObsoleteFiles "!insertmacro RemoveObsoleteFiles"

;-----------------------------------------

!macro WriteTheUninstaller

    ${DisableX64FSRedirection}

    DetailPrint "Creating uninstaller..."
    SetOutPath "$INSTDIR"

    WriteRegStr   HKLM ${UD_UNINSTALL_REG_KEY} "DisplayName"     "Ultra Defragmenter"
    !ifdef RELEASE_STAGE
        WriteRegStr   HKLM ${UD_UNINSTALL_REG_KEY} "DisplayVersion"  "$%ULTRADFGVER% ${RELEASE_STAGE}"
    !else
        WriteRegStr   HKLM ${UD_UNINSTALL_REG_KEY} "DisplayVersion"  "$%ULTRADFGVER%"
    !endif
    WriteRegStr   HKLM ${UD_UNINSTALL_REG_KEY} "Publisher"       "UltraDefrag Development Team"
    WriteRegStr   HKLM ${UD_UNINSTALL_REG_KEY} "URLInfoAbout"    "https://ultradefrag.net/"
    WriteRegStr   HKLM ${UD_UNINSTALL_REG_KEY} "UninstallString" "$INSTDIR\uninstall.exe"
    WriteRegStr   HKLM ${UD_UNINSTALL_REG_KEY} "DisplayIcon"     "$INSTDIR\uninstall.exe"
    WriteRegStr   HKLM ${UD_UNINSTALL_REG_KEY} "InstallLocation" "$INSTDIR"
    WriteRegDWORD HKLM ${UD_UNINSTALL_REG_KEY} "NoModify" 1
    WriteRegDWORD HKLM ${UD_UNINSTALL_REG_KEY} "NoRepair" 1

    WriteUninstaller "uninstall.exe"

    ${EnableX64FSRedirection}

!macroend

!define WriteTheUninstaller "!insertmacro WriteTheUninstaller"

;-----------------------------------------

!macro UpdateUninstallSizeValue
    DetailPrint "Calculating installation size..."

    push $R0
    push $0
    push $1
    push $2

    ${DisableX64FSRedirection}

    ; save estimated size of the installation to system registry
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKLM ${UD_UNINSTALL_REG_KEY} "EstimatedSize" "$0"

    ${EnableX64FSRedirection}

    pop $2
    pop $1
    pop $0
    pop $R0

!macroend

!define UpdateUninstallSizeValue "!insertmacro UpdateUninstallSizeValue"

;-----------------------------------------

!macro RegisterInstallationFolder

    ${DisableX64FSRedirection}

    WriteRegStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "UD_INSTALL_DIR" "$INSTDIR"

    ; "Export" our change
    SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000

    ${EnableX64FSRedirection}

!macroend

!define RegisterInstallationFolder "!insertmacro RegisterInstallationFolder"

;-----------------------------------------

!macro UnRegisterInstallationFolder

    ${DisableX64FSRedirection}

    DeleteRegValue HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "UD_INSTALL_DIR"
    DeleteRegValue HKLM "SYSTEM\ControlSet001\Control\Session Manager\Environment"     "UD_INSTALL_DIR"
    DeleteRegValue HKLM "SYSTEM\ControlSet002\Control\Session Manager\Environment"     "UD_INSTALL_DIR"

    ; "Export" our change
    SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000

    ${EnableX64FSRedirection}

!macroend

!define UnRegisterInstallationFolder "!insertmacro UnRegisterInstallationFolder"

;-----------------------------------------

!endif /* _ULTRA_DEFRAG_NSH_ */
