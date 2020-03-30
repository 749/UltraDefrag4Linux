#!/usr/local/bin/lua
--[[
  upgrade-rptopts.lua - script used to upgrade UltraDefrag report options.
  Copyright (c) 2012 Dmitri Arkhangelski (dmitriar@gmail.com).

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

-- USAGE: lua upgrade-rptopts.lua {UltraDefrag installation directory}

-- CONFIGURATION FILE
config_file_contents = [[
-------------------------------------------------------------------------------
-- Ultra Defragmenter report options
-- This file is written in Lua programming language http://www.lua.org/
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- Set this parameter to zero to disable HTML reports generation.
-------------------------------------------------------------------------------
produce_html_report = $produce_html_report

-------------------------------------------------------------------------------
-- Set this parameter to 1 to enable generation of plain text reports.
-------------------------------------------------------------------------------
produce_plain_text_report = $produce_plain_text_report

-------------------------------------------------------------------------------
-- All the following options were primarily designed to achieve better
-- compatibility with old web browsers.
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- Set enable_sorting to zero if your web browser is too old
-- and you have error messages about invalid javascript code.
-------------------------------------------------------------------------------
enable_sorting = $enable_sorting

-------------------------------------------------------------------------------
-- Set this parameter to 1 if you prefer to look at filenames split
-- into few short lines. If you prefer to use fullscreen mode of your
-- web browser then set this parameter to zero.
-------------------------------------------------------------------------------
split_long_names = $split_long_names

-------------------------------------------------------------------------------
-- Set here maximum number of characters per line in filename cells.
-------------------------------------------------------------------------------
max_chars_per_line = $max_chars_per_line

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- this number helps to upgrade configuration file correctly, don't change it
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
version = $current_version

-------------------------------------------------------------------------------
-- The web page style can be set through udreport.css style sheet.
-------------------------------------------------------------------------------
]]

function expand (s)
  s = string.gsub(s, "$([%w_]+)", function (n)
        return string.gsub(tostring(_G[n]), "\\", "\\\\")
      end)
  return s
end

function save_preferences(f)
    f:write(expand(config_file_contents))
end

-- THE MAIN CODE STARTS HERE
-- current version of configuration file
current_version = 1
old_version = 0
upgrade_needed = 1

-- parse command line
instdir = arg[1]
assert(instdir, "upgrade-rptopts.lua: the first argument is missing")

-- set defaults
produce_html_report = 1
produce_plain_text_report = 0
enable_sorting = 1
split_long_names = 0
max_chars_per_line = 50

-- get user preferences
path = instdir .. "\\options\\udreportopts.lua"
f = io.open(path, "r")
if f then
    f:close()
    dofile(path)
end
path = instdir .. "\\options\\udreportopts-custom.lua"
f = io.open(path, "r")
if f then
    f:close()
    dofile(path)
end

-- if version of configuration file is greater or equal than the current one, do nothing
if version then
    old_version = version
    if version >= current_version then
        upgrade_needed = 0
    end
end

-- upgrade the file
if upgrade_needed ~= 0 then
    -- make a backup copy
    path = instdir .. "\\options\\udreportopts-custom.lua"
    f = io.open(path, "r")
    if not f then
        path = instdir .. "\\options\\udreportopts.lua"
        f = io.open(path, "r")
    end
    if f then
        contents = f:read("*all")
        f:close()
        path = instdir .. "\\options\\udreportopts.lua.old"
        f = assert(io.open(path, "w"))
        f:write(contents)
        f:close()
    end

    -- save the upgraded configuration
    path = instdir .. "\\options\\udreportopts.lua"
    f = assert(io.open(path, "w"))
    save_preferences(f)
    f:close()
end
