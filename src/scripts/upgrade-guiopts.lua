#!/usr/local/bin/lua
--[[
  upgrade-guiopts.lua - script used to upgrade UltraDefrag GUI configuration file.
  Copyright (c) 2011 by Dmitri Arkhangelski (dmitriar@gmail.com).

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

-- USAGE: lua upgrade-guiopts.lua {UltraDefrag installation directory}

-- CONFIGURATION FILE
config_file_contents = [[
--------------------------------------------------------------------------------
-- UltraDefrag GUI Configuration file
-- This file is written in Lua programming language http://www.lua.org/
--------------------------------------------------------------------------------

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- I. Filters

-- The main goal of defragmentation is disk access speedup. However, while 
-- some fragmented files decrease its performance, another may be left as they
-- are without any noticeable system performance degradation.

-- To filter out files which take no effect on system performance, UltraDefrag
-- supports a few flexible filters.

-- in_filter and ex_filter can be used to exclude files by their paths.
-- For example, to defragment all mp3 files on disk except of these locating
-- inside temporary folders, the following pair of filters may be used:

-- in_filter = "*.mp3"
-- ex_filter = "*temp*;*tmp*"

-- Both filters support '?' and '*' wildcards. '?' character matches any one
-- character, '*' - any zero or more characters. For example, "file.mp?" matches
-- both file.mp3 and file.mp4 while "*.mp3" matches all files with mp3 extension.

-- *.* pattern matches any file with an extension. An asterisk alone (*) matches
-- anything (with or without an extension).

-- Note that you must type either full paths, or substitute the beginning of the
-- path by '*' wildcard. For example, to defragment all the books of a famous Scottish
-- novelist use "C:\\Books\\Arthur Conan Doyle\\*" or simply "*\\Arthur Conan Doyle\\*"
-- string.

-- Note that paths must be typed with double back slashes instead of the single
-- ones. For example, "C:\\MyDocs\\Music\\mp3\\Red_Hot_Chili_Peppers\\*"

-- Note that files marked as temporary by system are always excluded regardless
-- of filters, since these files usually take no effect on system performance.
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

in_filter = "$in_filter"
ex_filter = "$ex_filter"

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- The sizelimit filter allows to exclude big files from the
-- defragmentation. For example, when you watch a movie, it takes usually
-- 1-2 hours while time needed to move drive's head from one fragment to
-- another is about a few seconds. Therefore, you'll see no difference between 
-- fragmented and not fragmented movie file. By setting sizelimit filter, 
-- overall disk defragmentation time can be highly decreased.

-- To exclude all files greater than 100 Mb, set:

-- sizelimit = "100 Mb"
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

sizelimit = "$sizelimit"

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- The fragments_threshold filter allows to exclude files which have low
-- number of fragments. For example, to exclude everything with less than
-- 5 fragments, set:

-- fragments_threshold = 5
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

fragments_threshold = $fragments_threshold

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- II. Miscellaneous options
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- When the specified time interval elapses the job will be terminated 
-- automatically. For example, to terminate processing after 6 hours and 
-- 30 minutes, set:

-- time_limit = "6h 30m"
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

time_limit = "$time_limit"

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- Progress refresh interval, in milliseconds. The default value is 100. 
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

refresh_interval = $refresh_interval

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- Set it to 1 (one) to disable generation of the file fragmentation reports.
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

disable_reports = $disable_reports

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- Set dbgprint_level to DETAILED for reporting a bug,
-- for normal operation set it to NORMAL or an empty string.
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

dbgprint_level = "$dbgprint_level"

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- Set log_file_path to the path and file name of the log file to be created,
-- for normal operation set it to an empty string.
-- For example:
-- log_file_path = "C:\\Windows\\UltraDefrag\\Logs\\ultradefrag.log"
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

log_file_path = "$log_file_path"

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- Set dry_run parameter to 1 for defragmentation algorithm testing;
-- no actual data moves will be performed on disk in this case.
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

dry_run = $dry_run

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- Seconds_for_shutdown_rejection sets the delay for the user to cancel
-- the hibernate, logoff, reboot or shutdown execution, default is 60 seconds.
-- If set to 0 (zero) the confirmation dialog will not be displayed.
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

seconds_for_shutdown_rejection = $seconds_for_shutdown_rejection

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- Set disable_latest_version_check parameter to 1 to disable the automatic
-- check for the latest available version of the program on startup.
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

disable_latest_version_check = $disable_latest_version_check

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- Set scale_by_dpi parameter to 0 to not scale the buttons and text 
-- according to the screens DPI settings.
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

scale_by_dpi = $scale_by_dpi

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- Set restore_default_window_size parameter to 1
-- to restore default window size on the next startup.
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

restore_default_window_size = $restore_default_window_size

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- III. Cluster map options

-- map_block_size controls the size of the block, in pixels; default value is 4.
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

map_block_size = $map_block_size

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- Grid line width, in pixels; default value is 1.
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

grid_line_width = $grid_line_width

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- Grid line color, in RGB format; default value is (0;0;0),
-- all color components should be in range 0-255,
-- (0;0;0) means black; (255;255;255) - white.
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

grid_color_r = $grid_color_r
grid_color_g = $grid_color_g
grid_color_b = $grid_color_b

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- this number helps to upgrade configuration file correctly, don't change it
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

version = $current_version

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- END OF FILE
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
]]

int_config_file_contents = [[
-- the settings below are not changeable by the user,
-- they are always overwritten when the program ends
rx = $rx
ry = $ry
rwidth = $rwidth
rheight = $rheight
maximized = $maximized

skip_removable = $skip_removable
repeat_action = $repeat_action

column1_width = $column1_width
column2_width = $column2_width
column3_width = $column3_width
column4_width = $column4_width
column5_width = $column5_width
list_height = $list_height

job_flags = $job_flags
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

function save_internal_preferences(f)
    f:write(expand(int_config_file_contents))
end

-- THE MAIN CODE STARTS HERE
-- current version of configuration file
current_version = 1
old_version = 0
upgrade_needed = 1

-- parse command line
instdir = arg[1]
assert(instdir, "upgrade-guiopts.lua: the first argument is missing")

-- set defaults
in_filter = ""
ex_filter = "*system volume information*;*temp*;*tmp*;*recycle*;*.zip;*.7z;*.rar"
sizelimit = ""
fragments_threshold = 0
time_limit = ""
refresh_interval = 100
disable_reports = 0
dbgprint_level = ""
log_file_path = ""
dry_run = 0
seconds_for_shutdown_rejection = 60
disable_latest_version_check = 0
scale_by_dpi = 1
restore_default_window_size = 0
map_block_size = 4
grid_line_width = 1
grid_color_r = 0
grid_color_g = 0
grid_color_b = 0

-- get user preferences
path = instdir .. "\\options\\guiopts.lua"
f = io.open(path, "r")
if f ~= nil then
    f:close()
    dofile(path)
end

-- if version of configuration file is greater or equal than the current one, do nothing
if version ~= nil then
    old_version = version
    if version >= current_version then
        upgrade_needed = 0
    end
end

-- upgrade the file
if upgrade_needed ~= 0 then
    -- make a backup copy
    f = io.open(path, "r")
    if f ~= nil then
        contents = f:read("*all")
        f:close()
        f = assert(io.open(path .. ".old", "w"))
        if f ~= nil then
            f:write(contents)
            f:close()
        end
    end
    
    -- RULES OF UPGRADE TO THE CURRENT VERSION
    if old_version == 0 then
        -- revert in_filter and ex_filter to their defaults
        -- this is a main reason for upgrade to the version 1
        in_filter = ""
        ex_filter = "*system volume information*;*temp*;*tmp*;*recycle*;*.zip;*.7z;*.rar"
    end
    
    -- save the upgraded configuration
    f = assert(io.open(path, "w"))
    if f ~= nil then
        save_preferences(f)
        f:close()
    end
    
    -- save guiopts-internals.lua when needed
    if old_version == 0 then
        if rx ~= nil then
            f = assert(io.open(instdir .. "\\options\\guiopts-internals.lua", "w"))
            if f ~= nil then
                save_internal_preferences(f)
                f:close()
            end
        end
    end
end
