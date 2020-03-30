#!/usr/local/bin/lua
--[[
  upgrade-options.lua - upgrades UltraDefrag configuration file.
  Copyright (c) 2011-2018 Dmitri Arkhangelski (dmitriar@gmail.com).

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

-- USAGE: lua upgrade-options.lua {UltraDefrag installation directory}

-- CONFIGURATION FILE
config_file_contents = [[
-------------------------------------------------------------------------------
-- UltraDefrag configuration file
-------------------------------------------------------------------------------
-- This file is written in Lua programming language http://www.lua.org/
--
-- Use double back slashes instead of singe ones in strings (it's part of Lua
-- syntax), like so: in_filter = "c:\\windows\\*"
--
-- To use Unicode characters save this file in UTF-8 (without BOM) encoding
-- using Notepad++ editor (http://www.notepad-plus-plus.org/).
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- Common options
-------------------------------------------------------------------------------
-- These options have exactly the same meaning as environment variables
-- accepted by UltraDefrag console interface. Read the appropriate
-- chapter of the UltraDefrag Handbook for detailed information.
-- You should have received it along with this program; if not, go to
-- https://ultradefrag.net/handbook/
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- List of semicolon separated paths which need to be defragmented.
-- Examples:
--   in_filter = "*.jpg;*.png;*.gif" -- defragment pictures only
--   in_filter = "?:\\windows\\*"    -- defragment windows folders only
-- Keep it empty ("") or set to "*" to defragment everything.
-------------------------------------------------------------------------------

in_filter = "$in_filter"

-------------------------------------------------------------------------------
-- List of semicolon separated paths which need to be skipped,
-- i.e. left untouched.
-- Examples:
--   ex_filter = "*temp*;*tmp*;*recycle*"      -- exclude temporary content
--   ex_filter = "*.avi;*.mkv;*\\video_ts\\*"  -- exclude movies
-------------------------------------------------------------------------------

ex_filter = "$ex_filter"

-------------------------------------------------------------------------------
-- Popular patterns intended for inclusion in filters.
-- For more extensions visit http://www.fileinfo.com/
-------------------------------------------------------------------------------

archive_patterns = "$archive_patterns"
audio_patterns = "$audio_patterns"
disk_image_patterns = "$disk_image_patterns"
video_patterns = "$video_patterns"

-------------------------------------------------------------------------------
-- Set these variables to 1 to include patterns defined above to in_filter.
-------------------------------------------------------------------------------

include_archive = $include_archive
include_audio = $include_audio
include_disk_image = $include_disk_image
include_video = $include_video

-------------------------------------------------------------------------------
-- Set these variables to 1 to include patterns defined above to ex_filter.
-------------------------------------------------------------------------------

exclude_archive = $exclude_archive
exclude_audio = $exclude_audio
exclude_disk_image = $exclude_disk_image
exclude_video = $exclude_video

-------------------------------------------------------------------------------
-- Eliminate only fragments smaller than specified.
-- Example:
--   fragment_size_threshold = "20 MB"
-- Clean it up ("") or set to zero ("0") to eliminate all fragments.
-------------------------------------------------------------------------------

fragment_size_threshold = "$fragment_size_threshold"

-------------------------------------------------------------------------------
-- Exclude files larger than specified.
-- Example:
--   file_size_threshold = "100 MB"
-- Keep it empty ("") to defragment files of all sizes.
-------------------------------------------------------------------------------

file_size_threshold = "$file_size_threshold"

-------------------------------------------------------------------------------
-- In optimization, sort out files smaller than specified only.
-- Example:
--   optimizer_file_size_threshold = "20 MB"
-- When this parameter is set to zero ("0") or the empty string ("")
-- the default value shown above will be used to avoid nonsense.
-- When you increase this parameter keep in mind that UltraDefrag
-- needs larger free space gaps to sort bigger files out, thus
-- it may fail to do it perfectly.
-------------------------------------------------------------------------------

optimizer_file_size_threshold = "$optimizer_file_size_threshold"

-------------------------------------------------------------------------------
-- Exclude files having less fragments than specified.
-- Example:
--   fragments_threshold = 5 -- skip files having 4 fragments or less
-- Keep it empty ("") or set to zero ("0") to turn this filter off.
-- Note: for solid state drives (SSD) we recommend to set this parameter
-- to 20 to exclude slightly fragmented content which doesn't affect
-- performance.
-------------------------------------------------------------------------------

fragments_threshold = $fragments_threshold

-------------------------------------------------------------------------------
-- Skip disks entirely when their fragmentation level is below than specified.
-- Example:
--   fragmentation_threshold = 10 -- ignore fragmentation level below 10%
-- Set it to zero (0) to process disks regardless of their fragmentation level.
-- Note: this parameter doesn't affect the MFT optimization task.
-------------------------------------------------------------------------------

fragmentation_threshold = $fragmentation_threshold

-------------------------------------------------------------------------------
-- Terminate the job when the specified time interval elapses.
-- Example:
--   time_limit = "6h 30m"
-- Keep it empty ("") or set to zero ("0") to ignore timing.
-------------------------------------------------------------------------------

time_limit = "$time_limit"

-------------------------------------------------------------------------------
-- The progress refresh interval, in milliseconds. The default value is 100.
-------------------------------------------------------------------------------

refresh_interval = $refresh_interval

-------------------------------------------------------------------------------
-- Set it to 1 (one) to disable generation of the file fragmentation reports.
-------------------------------------------------------------------------------

disable_reports = $disable_reports

-------------------------------------------------------------------------------
-- Set it to 1 to avoid physical movements of files, i.e. to simulate
-- the disk processing. This allows to check out algorithms quickly.
-------------------------------------------------------------------------------

dry_run = $dry_run

-------------------------------------------------------------------------------
-- Set it to DETAILED for troubleshooting, otherwise keep it empty ("")
-- or set to NORMAL. Note that the detailed logging consumes more time,
-- memory and disk space.
-------------------------------------------------------------------------------

dbgprint_level = "$dbgprint_level"

-------------------------------------------------------------------------------
-- Set it to redirect debugging output to a log file. Keep it
-- empty ("") if everything works fine and no logging is needed.
-- Examples:
--   log_file_path = "C:\\Windows\\UltraDefrag\\logs\\ultradefrag.log"
--   log_file_path = ".\\logs\\ultradefrag.log"
-- Environment variables can be used as shown below:
--   log_file_path = os.getenv("TEMP") .. "\\UltraDefrag_Logs\\ultradefrag.log"
-- Note:
--   Unicode characters cannot be included into the log file path.
-------------------------------------------------------------------------------

log_file_path = "$log_file_path"

-------------------------------------------------------------------------------
-- Context menu entries in Windows Explorer
-------------------------------------------------------------------------------
-- These options control what happens when you defragment
-- files through their context menu in Windows Explorer.
-------------------------------------------------------------------------------

if shellex_flag then
    -- the context menu handler takes into account everything defined above
    -- as well as options defined here exclusively for it; to pass sorting
    -- options to the context menu handler use the following code:
    --   os.setenv("UD_SORTING","PATH")
    --   os.setenv("UD_SORTING_ORDER","ASC")
    -- refer to the Console chapter of UltraDefrag Handbook for details $shellex_options
end

-------------------------------------------------------------------------------
-- Graphical interface
-------------------------------------------------------------------------------
-- These options control look and feel of the graphical interface.
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- Set it to 1 to minimize the application's window
-- to the taskbar notification area (system tray).
-------------------------------------------------------------------------------

minimize_to_system_tray = $minimize_to_system_tray

-------------------------------------------------------------------------------
-- Set it to 1 to show a taskbar icon overlay on Windows 7 and more recent
-- Windows editions. This overlay will reflect the state of the running job
-- in real time.
-------------------------------------------------------------------------------

show_taskbar_icon_overlay = $show_taskbar_icon_overlay

-------------------------------------------------------------------------------
-- Set it to 1 to enable progress indication inside of the taskbar
-- button on Windows 7 and more recent Windows editions.
-------------------------------------------------------------------------------

show_progress_in_taskbar = $show_progress_in_taskbar

-------------------------------------------------------------------------------
-- Set it to zero if menu icons look untidy on your system.
-- Note: restart the program after adjustment of this parameter.
-------------------------------------------------------------------------------

show_menu_icons = $show_menu_icons

-------------------------------------------------------------------------------
-- Size of the map cells, in pixels. The default value is 4.
-------------------------------------------------------------------------------

map_block_size = $map_block_size

-------------------------------------------------------------------------------
-- Width of the grid line, in pixels. The default value is 1.
-------------------------------------------------------------------------------

grid_line_width = $grid_line_width

-------------------------------------------------------------------------------
-- The grid line color, in RGB format. Black (0;0;0) is used by default,
-- visit http://www.colorpicker.com/ to pick others.
-------------------------------------------------------------------------------

grid_color_r = $grid_color_r
grid_color_g = $grid_color_g
grid_color_b = $grid_color_b

-------------------------------------------------------------------------------
-- The free space color, in RGB format. White (255;255;255) is used by default,
-- visit http://www.colorpicker.com/ to pick others.
-------------------------------------------------------------------------------

free_color_r = $free_color_r
free_color_g = $free_color_g
free_color_b = $free_color_b

-------------------------------------------------------------------------------
-- The shutdown confirmation timeout, in seconds. The default value
-- is 60, set it to zero to skip any confirmation. Note: it works
-- the same way when you confirm hibernation, logoff or reboot.
-------------------------------------------------------------------------------

seconds_for_shutdown_rejection = $seconds_for_shutdown_rejection

-------------------------------------------------------------------------------
-- The following options have been retired:
--
--   � disable_latest_version_check - use the "Help > Upgrade" menu instead
--   � restore_default_window_size  - remove the width and height parameters
--     from the {installation folder}\gui.ini file manually to restore
--     the default window size on the next startup
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- File fragmentation reports
-------------------------------------------------------------------------------
-- These options define what happens when you open a file fragmentation
-- report, either from the graphical interface or from Windows Explorer.
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- Set it to zero to disable HTML reports generation.
-------------------------------------------------------------------------------

produce_html_report = $produce_html_report

-------------------------------------------------------------------------------
-- Set it to 1 to enable generation of plain text reports.
-------------------------------------------------------------------------------

produce_plain_text_report = $produce_plain_text_report

-------------------------------------------------------------------------------
-- Set it to 1 to split long paths in HTML reports to few
-- shorter lines (for better appearance on small screens).
-------------------------------------------------------------------------------

split_long_names = $split_long_names

-------------------------------------------------------------------------------
-- Maximum number of characters per line. The default value is 50.
-- Note: this parameter is ignored when split_long_names is set to 0.
-------------------------------------------------------------------------------

max_chars_per_line = $max_chars_per_line

-------------------------------------------------------------------------------
-- To adjust style of HTML reports edit the following file:
-- {installation folder}\scripts\udreport.css
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- this number helps to upgrade configuration file correctly, don't change it
-------------------------------------------------------------------------------

version = $current_version

-------------------------------------------------------------------------------
-- this code concatenates filter variables, don't modify it
-------------------------------------------------------------------------------

orig_ex_filter = ex_filter  -- for faster upgrade
if exclude_archive ~= 0 then ex_filter = ex_filter .. ";" .. archive_patterns end
if exclude_audio ~= 0 then ex_filter = ex_filter .. ";" .. audio_patterns end
if exclude_disk_image ~= 0 then ex_filter = ex_filter .. ";" .. disk_image_patterns end
if exclude_video ~= 0 then ex_filter = ex_filter .. ";" .. video_patterns end

orig_in_filter = in_filter  -- for faster upgrade
if include_archive ~= 0 then in_filter = in_filter .. ";" .. archive_patterns end
if include_audio ~= 0 then in_filter = in_filter .. ";" .. audio_patterns end
if include_disk_image ~= 0 then in_filter = in_filter .. ";" .. disk_image_patterns end
if include_video ~= 0 then in_filter = in_filter .. ";" .. video_patterns end

-------------------------------------------------------------------------------
-- this code sets environment variables for UltraDefrag, don't modify it
-------------------------------------------------------------------------------

-- common variables
os.setenv("UD_IN_FILTER",in_filter)
os.setenv("UD_EX_FILTER",ex_filter)
os.setenv("UD_FRAGMENT_SIZE_THRESHOLD",fragment_size_threshold)
os.setenv("UD_FILE_SIZE_THRESHOLD",file_size_threshold)
os.setenv("UD_OPTIMIZER_FILE_SIZE_THRESHOLD",optimizer_file_size_threshold)
os.setenv("UD_FRAGMENTS_THRESHOLD",fragments_threshold)
os.setenv("UD_FRAGMENTATION_THRESHOLD",fragmentation_threshold)
os.setenv("UD_TIME_LIMIT",time_limit)
os.setenv("UD_REFRESH_INTERVAL",refresh_interval)
os.setenv("UD_DISABLE_REPORTS",disable_reports)
os.setenv("UD_DBGPRINT_LEVEL",dbgprint_level)
os.setenv("UD_LOG_FILE_PATH",log_file_path)
os.setenv("UD_DRY_RUN",dry_run)

-- GUI specific variables
os.setenv("UD_SECONDS_FOR_SHUTDOWN_REJECTION",seconds_for_shutdown_rejection)
os.setenv("UD_SHOW_MENU_ICONS",show_menu_icons)
os.setenv("UD_SHOW_TASKBAR_ICON_OVERLAY",show_taskbar_icon_overlay)
os.setenv("UD_SHOW_PROGRESS_IN_TASKBAR",show_progress_in_taskbar)
os.setenv("UD_MINIMIZE_TO_SYSTEM_TRAY",minimize_to_system_tray)
os.setenv("UD_MAP_BLOCK_SIZE",map_block_size)
os.setenv("UD_GRID_LINE_WIDTH",grid_line_width)
os.setenv("UD_GRID_COLOR_R",grid_color_r)
os.setenv("UD_GRID_COLOR_G",grid_color_g)
os.setenv("UD_GRID_COLOR_B",grid_color_b)
os.setenv("UD_FREE_COLOR_R",free_color_r)
os.setenv("UD_FREE_COLOR_G",free_color_g)
os.setenv("UD_FREE_COLOR_B",free_color_b)

-------------------------------------------------------------------------------
-- END OF FILE
-------------------------------------------------------------------------------
]]

function expand(s)
  s = string.gsub(s, "$([%w_]+)", function (n)
        return string.gsub(tostring(_G[n]), "\\", "\\\\")
      end)
  return s
end

function save_preferences(f)
    f:write(expand(config_file_contents))
end

function extract_preferences(file)
    local path = instdir .. "\\" .. file
    local f = io.open(path, "r")
    if f then
        f:close()
        dofile(path)
    end
end

function get_preferences()
    -- the version of the old configuration file
    version = 0
    
    -- set defaults
    in_filter = ""
    ex_filter = "*system volume information*;*temp*;*tmp*;*recycle*;*dllcache*;*ServicePackFiles*"
    archive_patterns = "*.7z;*.7z.*;*.arj;*.bz2;*.bzip2;*.cab;*.cpio;*.deb;*.dmg;*.gz;*.gzip;*.lha;*.lzh;*.lzma"
    archive_patterns = archive_patterns .. ";*.rar;*.rpm;*.swm;*.tar;*.taz;*.tbz;*.tbz2;*.tgz;*.tpz;*.txz"
    archive_patterns = archive_patterns .. ";*.xar;*.xz;*.z;*.zip"
    audio_patterns = "*.aif;*.cda;*.flac;*.iff;*.kpl;*.m3u;*.m4a;*.mid;*.mp3;*.mpa;*.ra;*.wav;*.wma"
    disk_image_patterns = "*.fat;*.hdd;*.hfs;*.img;*.iso;*.ntfs;*.squashfs;*.vdi;*.vhd;*.vmdk;*.wim"
    video_patterns = "*.3g2;*.3gp;*.asf;*.asx;*.avi;*.flv;*.mov;*.mp4;*.mpg;*.rm;*.srt;*.swf;*.vob;*.wmv"
    include_archive = 0
    exclude_archive = 0
    include_audio = 0
    exclude_audio = 0
    include_disk_image = 0
    exclude_disk_image = 0
    include_video = 0
    exclude_video = 0
    fragment_size_threshold = "20 MB"
    sizelimit = ""
    optimizer_file_size_threshold = "20 MB"
    fragments_threshold = 0
    fragmentation_threshold = 0
    time_limit = ""
    refresh_interval = 100
    disable_reports = 0
    dbgprint_level = ""
    log_file_path = ".\\logs\\ultradefrag.log"
    dry_run = 0
    seconds_for_shutdown_rejection = 60
    show_menu_icons = 1
    show_taskbar_icon_overlay = 1
    show_progress_in_taskbar = 1
    minimize_to_system_tray = 0
    map_block_size = 4
    grid_line_width = 1
    grid_color_r = 0
    grid_color_g = 0
    grid_color_b = 0
    free_color_r = 255
    free_color_g = 255
    free_color_b = 255
    
    produce_html_report = 1
    produce_plain_text_report = 0
    split_long_names = 0
    max_chars_per_line = 50
    
    -- get user preferences
    path = instdir .. "\\conf\\options.lua"
    f = io.open(path, "r")
    if f then
        f:close()
        dofile(path)
    else
        -- try to gain something from obsolete configuration files
        extract_preferences("options\\guiopts.lua")
        extract_preferences("options\\udreportopts.lua")
        extract_preferences("options\\udreportopts-custom.lua")
        extract_preferences("options.lua")
    end

    -- upgrade preferences
    if version == 0 then
        -- upgrade filters, now they should include wildcards
        in_filter = ""
        ex_filter = "*system volume information*;*temp*;*tmp*;*recycle*;*dllcache*;*ServicePackFiles*"
    end
    if not file_size_threshold then
        -- sizelimit has been superseded by file_size_threshold
        file_size_threshold = sizelimit
    end
    local path_upgrade_needed = 0
    if version < 8 then path_upgrade_needed = 1 end
    if version > 99 and version < 107 then path_upgrade_needed = 1 end
    if path_upgrade_needed ~= 0 and log_file_path == "" then
        -- enable logging to file, to simplify troubleshooting
        log_file_path = ".\\logs\\ultradefrag.log"
    end
    if orig_in_filter then in_filter = orig_in_filter end
    if orig_ex_filter then ex_filter = orig_ex_filter end
end

-- THE MAIN CODE STARTS HERE
-- the current version of the configuration file
-- 0 - 99 for v5; 100 - 199 for v6; 200+ for v7+
current_version = 206
shellex_options = ""
_G_copy = {}

-- parse command line
instdir = arg[1]
assert(instdir, "upgrade-options.lua: the first argument is missing")

-- get preferences for the context menu handler
shellex_flag = 1
get_preferences()
for k,v in pairs(_G) do _G_copy[k] = v end

-- get preferences for the graphical interface
shellex_flag = nil
get_preferences()

-- shellex_options = difference between two sets of preferences
for k,v in pairs(_G) do
    local t = type(v)
    if t == 'string' or t == 'number' or t == 'boolean' then
        if _G_copy[k] ~= _G[k] and k ~= 'shellex_options' then
            shellex_options = shellex_options .. '\n    ' .. k .. ' = '
            if t == 'string' then shellex_options = shellex_options .. '\"' end
            shellex_options = shellex_options .. _G_copy[k]
            if t == 'string' then shellex_options = shellex_options .. '\"' end
        end
    end
end

-- initially custom options should redefine the log path
if version < 114 then
    shellex_options = '\n    log_file_path = \"'
    if os.getenv('UD_INSTALL_DIR') then
        shellex_options = shellex_options .. os.getenv('UD_INSTALL_DIR') .. '\\logs\\'
    else
        shellex_options = shellex_options .. os.getenv('TMP') .. '\\UltraDefrag_Logs\\'
    end
    shellex_options = shellex_options .. 'udefrag-shellex.log\"'
end

-- upgrade configuration file when needed
if version < current_version then
    path = instdir .. "\\conf\\options.lua"
    -- make a backup copy
    f = io.open(path, "r")
    if f then
        contents = f:read("*all")
        f:close()
        f = assert(io.open(path .. ".old", "w"))
        f:write(contents)
        f:close()
    end

    -- save the upgraded configuration
    f = assert(io.open(path, "w"))
    save_preferences(f)

    -- flush configuration file to the disk
    -- right now to prepare it for tracking
    -- via FindFirstChangeNotification()
    f:flush()

    f:close()
end
