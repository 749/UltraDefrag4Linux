--[[
This is the Ultra Defragmenter build configurator.
Copyright (c) 2007-2013 Dmitri Arkhangelski (dmitriar@gmail.com).

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
WINDDKBASE    = ""
WINSDKBASE    = ""
MINGWBASE     = ""
MINGWx64BASE  = ""
NSISDIR       = ""
SEVENZIP_PATH = ""
mingw_patch    = 0
mingw_projects = 0
winsdk_patch   = 0

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
        dialog.size = "400x215"
    end
    return 1
end

-- show dialog box
ret, ULTRADFGVER, RELEASE_STAGE, MINGWBASE, NSISDIR, SEVENZIP_PATH, WINDDKBASE, WINSDKBASE, MINGWx64BASE, mingw_patch, winsdk_patch, mingw_projects = 
    iup.GetParam("UltraDefrag build configurator",param_action,
        "UltraDefrag version: %s\n"..
        "Release stage (alpha1, beta2, rc3, final): %s\n".. 
        "MinGW path: %s\n"..
        "NSIS path: %s\n"..
        "7-Zip path: %s\n"..
        "Windows Driver Kit v7.1.0 path: %s\n"..
        "Windows SDK base path: %s\n"..
        "MinGW x64 base path: %s\n"..
        "Apply patch to MinGW: %b[No,Yes]\n"..
        "Apply patch to Windows SDK: %b[No,Yes]\n"..
        "Make MinGW Developer Studio projects: %b[No,Yes]\n",
        ULTRADFGVER, RELEASE_STAGE, MINGWBASE, NSISDIR, SEVENZIP_PATH, WINDDKBASE, WINSDKBASE, MINGWx64BASE, mingw_patch, winsdk_patch, mingw_projects
        )
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
    if mingw_patch == 1 then
        print("\n")
        print("Apply patch to MinGW option was selected.")
        print("-----------------------------------------")
        if os.execute("cmd.exe /C .\\dll\\zenwinx\\mingw_patch.cmd \"" .. MINGWBASE .. "\"") ~= 0 then
            error("Cannot apply patch to MinGW!")
        end
    end
    if winsdk_patch == 1 then
        print("\n")
        print("Apply patch to Windows SDK option was selected.")
        print("-----------------------------------------------")
        if os.execute("cmd.exe /C .\\dll\\zenwinx\\winsdk_patch.cmd \"" .. WINSDKBASE .. "\"") ~= 0 then
            error("Cannot apply patch to Windows SDK!")
        end
    end
    if mingw_projects == 1 then
        print("\n")
        print("Make MinGW Developer Studio projects option was selected.")
        print("---------------------------------------------------------")
        if os.execute("lua tools\\make-mingw-projects.lua") ~= 0 then
            error("Cannot make MinGW Developer Studio projects!")
        end
    end
end
