--[[
This is the Ultra Defragmenter build configurator.
Copyright (c) 2007-2012 Dmitri Arkhangelski (dmitriar@gmail.com).

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
--]]

-- This program requires IUP and CD libraries.
-- http://sourceforge.net/projects/iup/
-- http://sourceforge.net/projects/canvasdraw/
require "iuplua51"
require "iupluaimglib51"
require "iupluacontrols51"
iup.SetLanguage("ENGLISH")

-- initialize parameters
ULTRADFGVER   = ""
RELEASE_STAGE = ""
MINGWBASE     = ""
NSISDIR       = ""
SEVENZIP_PATH = ""
WINDDKBASE    = ""
MINGWx64BASE  = ""
WINSDKBASE    = ""
apply_patch   = 0

ver_mj, ver_mn, ver_fix = 0,0,0

script = [[
@echo off
echo Set common environment variables...
set VERSION=$ver_mj,$ver_mn,$ver_fix,0
set VERSION2="$ver_mj, $ver_mn, $ver_fix, 0\0"
set ULTRADFGVER=$ULTRADFGVER
set RELEASE_STAGE=$RELEASE_STAGE
set UDVERSION_SUFFIX=$UDVERSION_SUFFIX
set WINDDKBASE=$WINDDKBASE
set WINSDKBASE=$WINSDKBASE
set MINGWBASE=$MINGWBASE
set MINGWx64BASE=$MINGWx64BASE
set NSISDIR=$NSISDIR
set SEVENZIP_PATH=$SEVENZIP_PATH
]]

function expand (s)
  s = string.gsub(s, "$([%w_]+)", function (n)
        return tostring(_G[n])
      end)
  return s
end

show_obsolete_options = 0
if arg[1] == "--all" then
    show_obsolete_options = 1
end

-- get parameters from setvars.cmd file
for line in io.lines("setvars.cmd") do
    i, j, key, value = string.find(line,"^%s*set%s*(.-)%s*=%s*(.-)%s*$")
    if key and value then
        if _G[key] then _G[key] = value end
    end
end

-- dialog box procedure
function param_action(dialog, param_index)
    if param_index == -2 then
        -- dialog initialization
        icon = iup.image {
            { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
            { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
            { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
            { 1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,1 },
            { 1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1 },
            { 1,1,1,2,1,1,1,2,2,1,1,1,1,1,1,1 },
            { 1,1,1,2,2,1,1,1,2,2,1,1,1,1,1,1 },
            { 1,1,1,1,2,2,1,1,2,2,1,1,1,1,1,1 },
            { 1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,1 },
            { 1,1,1,1,1,1,2,2,2,2,2,1,1,1,1,1 },
            { 1,1,1,1,1,1,1,1,1,2,2,2,1,1,1,1 },
            { 1,1,1,1,1,1,1,1,1,1,2,2,2,1,1,1 },
            { 1,1,1,1,1,1,1,1,1,1,1,2,2,1,1,1 },
            { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
            { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
            { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }
            ; colors = { "255 255 0", "255 0 0" }
        }
        dialog.icon = icon
        if show_obsolete_options == 1 then
            dialog.size = "400x188"
        else
            dialog.size = "370x158"
        end
    end
    return 1
end

-- show dialog box
if show_obsolete_options == 1 then
    ret, ULTRADFGVER, RELEASE_STAGE, MINGWBASE, NSISDIR, SEVENZIP_PATH, WINDDKBASE, MINGWx64BASE, WINSDKBASE, apply_patch = 
        iup.GetParam("UltraDefrag build configurator",param_action,
            "UltraDefrag version: %s\n"..
            "Release stage (alpha1, beta2, rc3, final): %s\n".. 
            "MinGW path: %s\n"..
            "NSIS path: %s\n"..
            "7-Zip path: %s\n"..
            "Windows Server 2003 DDK path: %s\n"..
            "MinGW x64 base path: %s\n"..
            "Windows SDK base path: %s\n"..
            "Apply patch to MinGW: %b[No,Yes]\n",
            ULTRADFGVER, RELEASE_STAGE, MINGWBASE, NSISDIR, SEVENZIP_PATH, WINDDKBASE, MINGWx64BASE, WINSDKBASE, apply_patch
            )
else
    ret, ULTRADFGVER, RELEASE_STAGE, MINGWBASE, NSISDIR, SEVENZIP_PATH, WINDDKBASE, apply_patch = 
        iup.GetParam("UltraDefrag build configurator",param_action,
            "UltraDefrag version: %s\n"..
            "Release stage (alpha1, beta2, rc3, final): %s\n".. 
            "MinGW path: %s\n"..
            "NSIS path: %s\n"..
            "7-Zip path: %s\n"..
            "Windows Server 2003 DDK path: %s\n"..
            "Apply patch to MinGW: %b[No,Yes]\n",
            ULTRADFGVER, RELEASE_STAGE, MINGWBASE, NSISDIR, SEVENZIP_PATH, WINDDKBASE, apply_patch
            )
end
if ret == 1 then
    -- save options
    i, j, ver_mj, ver_mn, ver_fix = string.find(ULTRADFGVER,"(%d+).(%d+).(%d+)")
    if RELEASE_STAGE == "final" then
        -- set variable for pre-release stages only
        RELEASE_STAGE = ""; UDVERSION_SUFFIX = ULTRADFGVER
    else
        UDVERSION_SUFFIX = ULTRADFGVER .. "-" .. RELEASE_STAGE
    end
    f = assert(io.open("setvars.cmd","w"))
    f:write(expand(script))
    f:close()
    print("setvars.cmd script was updated successfully.")
    if apply_patch == 1 then
        print("Apply MinGW patch option was selected.")
        if os.execute("cmd.exe /C .\\dll\\zenwinx\\mingw_patch.cmd " .. MINGWBASE) ~= 0 then
            error("Cannot apply patch to MinGW!")
        end
    end
end
