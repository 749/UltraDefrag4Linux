/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2013 Dmitri Arkhangelski (dmitriar@gmail.com).
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
* Language selection routines.
* Intended for the regular installer.
*/

!ifndef _LANGUAGE_SELECTOR_NSH_
!define _LANGUAGE_SELECTOR_NSH_

/*
* How to use it:
* 1. Insert ${InitLanguageSelector}
*    to .onInit function.
* 2. Use LANG_PAGE macro to add the
*    language selection page to the
*    installer.
*/

; --- support command line parsing
!include "FileFunc.nsh"
!insertmacro GetParameters
!insertmacro GetOptions

Var LanguagePack
ReserveFile "lang.ini"

/*
 * This collects the previous language setting
 */
!macro CollectOldLang

    Push $R0

    ${DisableX64FSRedirection}

    ClearErrors
    ReadRegStr $R0 HKLM "Software\UltraDefrag" "Language"
    ${Unless} ${Errors}
        WriteINIStr "$INSTDIR\lang.ini" "Language" "Selected" $R0
        FlushINI "$INSTDIR\lang.ini"
    ${EndUnless}

    SetRegView 64
    ClearErrors
    ReadRegStr $R0 HKLM "Software\UltraDefrag" "Language"
    ${Unless} ${Errors}
        WriteINIStr "$INSTDIR\lang.ini" "Language" "Selected" $R0
        FlushINI "$INSTDIR\lang.ini"
    ${EndUnless}
    SetRegView 32

    ${EnableX64FSRedirection}

    Pop $R0

!macroend

;-----------------------------------------------------------
;         LANG_PAGE macro and support routines
;-----------------------------------------------------------

!macro LANG_PAGE
    Page custom LangShow LangLeave ""
!macroend

;-----------------------------------------------------------

Function LangShow

    Push $R0
    Push $R1
    Push $R2

    StrCpy $LanguagePack "English (US)"

    ; --- get language from $INSTDIR\lang.ini file
    ${DisableX64FSRedirection}

    ; move old lang.ini to new location
    IfFileExists "$OldInstallDir\lang.ini" 0 SkipMove
    StrCmp "$INSTDIR" "$OldInstallDir" SkipMove

    CreateDirectory "$INSTDIR"
    Delete "$INSTDIR\lang.ini"
    ClearErrors
    Rename "$OldInstallDir\lang.ini" "$INSTDIR\lang.ini"
    ${If} ${Errors}
        MessageBox MB_OK|MB_ICONINFORMATION \
            "Cannot retrieve old language setting!" \
            /SD IDOK
    ${EndIf}

SkipMove:
    ${If} ${FileExists} "$INSTDIR\lang.ini"
        ReadINIStr $LanguagePack "$INSTDIR\lang.ini" "Language" "Selected"
    ${EndIf}
    ${EnableX64FSRedirection}

    ; --- get language from command line
    ; --- allows silent installation with: installer.exe /S /LANG="German"
    ${GetParameters} $R1
    ClearErrors
    ${GetOptions} $R1 /LANG= $R2
    ${Unless} ${Errors}
        StrCpy $LanguagePack $R2
    ${EndUnless}

    ${If} $LanguagePack == "French (FR)"
        StrCpy $LanguagePack "French"
    ${EndIf}
    ${If} $LanguagePack == "Vietnamese (VI)"
        StrCpy $LanguagePack "Vietnamese"
    ${EndIf}
    ${If} $LanguagePack == "Bosanski"
        StrCpy $LanguagePack "Bosnian"
    ${EndIf}

    WriteINIStr "$PLUGINSDIR\lang.ini" "Field 2" "State" $LanguagePack
    FlushINI "$PLUGINSDIR\lang.ini"

    InstallOptions::initDialog /NOUNLOAD "$PLUGINSDIR\lang.ini"
    Pop $R0
    InstallOptions::show
    Pop $R0

    Pop $R2
    Pop $R1
    Pop $R0
    Abort

FunctionEnd

;-----------------------------------------------------------

/**
 * @note Disables the x64 file system redirection.
 */
!macro InitLanguageSelector

    ${EnableX64FSRedirection}
    InitPluginsDir
    !insertmacro MUI_INSTALLOPTIONS_EXTRACT "lang.ini"

!macroend

;-----------------------------------------------------------

!define InitLanguageSelector "!insertmacro InitLanguageSelector"
!define CollectOldLang "!insertmacro CollectOldLang"

;-----------------------------------------------------------

Function LangLeave

    Push $R0

    ReadINIStr $R0 "$PLUGINSDIR\lang.ini" "Settings" "State"
    ${If} $R0 != "0"
        Pop $R0
        Abort
    ${EndIf}

    ; save selected language to $INSTDIR\lang.ini file before exit
    ReadINIStr $LanguagePack "$PLUGINSDIR\lang.ini" "Field 2" "State"
    ${DisableX64FSRedirection}
    CreateDirectory "$INSTDIR"
    WriteINIStr "$INSTDIR\lang.ini" "Language" "Selected" $LanguagePack
    FlushINI "$INSTDIR\lang.ini"
    ${EnableX64FSRedirection}
    Pop $R0

FunctionEnd

;-----------------------------------------------------------

!endif /* _LANGUAGE_SELECTOR_NSH_ */
