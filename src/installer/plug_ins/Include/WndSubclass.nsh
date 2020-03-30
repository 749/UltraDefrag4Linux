!ifndef _WNDSUBCLASS__INC
!define _WNDSUBCLASS__INC

!ifdef NSIS_VERSION
!if ! "${NSIS_VERSION}" == "2.15" ;2.15 is the only version without "v" prefix
!define /date __WNDSUBCLASS_NSISV %${NSIS_VERSION}
!if "${__WNDSUBCLASS_NSISV}" >= 2.42
!define __WNDSUBCLASS_NSISVEROK
!endif
!undef __WNDSUBCLASS_NSISV
!endif 
!endif
!ifndef __WNDSUBCLASS_NSISVEROK
!error "NSIS v2.42 and later required!"
!endif
!undef __WNDSUBCLASS_NSISVEROK





/* ********************
** WndSubclass_Subclass
** --------------------
Sets error flag on error
*/
!define WndSubclass_Subclass '!insertmacro _WndSubclass_Subclass GetFunctionAddress '
!define WndSubclass_Subclass_LabelCallback '!insertmacro _WndSubclass_Subclass GetLabelAddress '
!macro _WndSubclass_Subclass _func _hwnd _CB _tmpvar _outvar
${_func} ${_tmpvar} ${_CB}
WndSubclass::S ${_hwnd} ${_tmpvar}
pop ${_outvar}
!macroend


!define WndSubClass_Ret "!insertmacro _WndSubClass_Ret "
!macro _WndSubClass_Ret _retval
!if `${_retval}` != `$0`
StrCpy $0 ${_retval}
!endif
SetAutoClose true
return
!macroend


/* ***************************
** WndSubClass_CallNextWndProc
** ---------------------------
LRESULT in $0
*/
!define WndSubClass_CallNextWndProc "!insertmacro _WndSubClass_CallNextWndProc "
!macro _WndSubClass_CallNextWndProc _subclsid _hwnd _msg _wp _lp
WndSubclass::C "${_subclsid},${_hwnd},${_msg},${_wp},${_lp}"
!macroend




!endif ;_WNDSUBCLASS__INC
