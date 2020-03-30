#!/usr/local/bin/lua
--[[
  upgrade-guiopts.lua - script used to upgrade UltraDefrag GUI configuration file.
  Copyright (c) 2011-2012 Dmitri Arkhangelski (dmitriar@gmail.com).

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
--
-- To use Unicode characters in filters and other strings, edit this file
-- in Notepad++ editor (http://www.notepad-plus-plus.org/) and save it in
-- UTF-8 (without BOM) encoding.
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

-- Empty strings ("") turn off the filters.

-- Note that files marked as temporary by system are always excluded regardless
-- of filters, since these files usually take no effect on system performance.
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

in_filter = "$in_filter"
ex_filter = "$ex_filter"

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- The following ..._patterns variables allow to define groups of patterns
-- for inclusion in the in/ex_filter variables.

-- For more file extensions see http://www.fileinfo.com/
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

archive_patterns = "$archive_patterns"
audio_patterns = "$audio_patterns"
disk_image_patterns = "$disk_image_patterns"
video_patterns = "$video_patterns"

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- The following flag variables allow to specify if the pattern groups
-- defined above are to be added to the in_filter or ex_filter.

-- To add the group defined by archive_patterns to in_filter,
-- you set include_archive to 1.
-- Example: include_archive = 1

-- To add the group defined by archive_patterns to ex_filter,
-- you set exclude_archive to 1.
-- Example: exclude_archive = 1
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_archive = $include_archive
exclude_archive = $exclude_archive

include_audio = $include_audio
exclude_audio = $exclude_audio

include_disk_image = $include_disk_image
exclude_disk_image = $exclude_disk_image

include_video = $include_video
exclude_video = $exclude_video

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- The file_size_threshold filter allows to exclude big files from
-- the defragmentation. For example, when you watch a movie, it takes
-- usually 1-2 hours while time needed to move drive's head from one
-- fragment to another is about a few seconds. Therefore, you'll see
-- no difference between fragmented and not fragmented movie file.
-- By setting the file_size_threshold filter, overall disk
-- defragmentation time can be highly decreased.

-- To exclude all files greater than 100 Mb, set:

-- file_size_threshold = "100 Mb"
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

file_size_threshold = "$file_size_threshold"

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- The fragments_threshold filter allows to exclude files which have low
-- number of fragments. For example, to exclude everything with less than
-- 5 fragments, set:

-- fragments_threshold = 5

-- Both zero value and empty string ("") turn off the filter.
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

fragments_threshold = $fragments_threshold

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- The fragmentation_threshold filter allows to avoid the disk processing
-- when the disk fragmentation level is below than specified. For example,
-- to avoid defragmentation/optimization of disks with fragmentation level
-- below 10 percents, set:

-- fragmentation_threshold = 10

-- The default value is zero (0), so all the disks are processed
-- regardless of their fragmentation level.

-- Note that this filter does not affect the MFT optimization task.
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

fragmentation_threshold = $fragmentation_threshold

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- II. Miscellaneous options
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- When the specified time interval elapses the job will be terminated
-- automatically. For example, to terminate processing after 6 hours and
-- 30 minutes, set:

-- time_limit = "6h 30m"

-- Both zero value and empty string ("") turn off this option.
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
-- for normal operation set it to an empty string ("").
-- For example:
--     log_file_path = "C:\\Windows\\UltraDefrag\\Logs\\ultradefrag.log"
--
--  Same as above, but uses relative path
--    log_file_path = ".\\Logs\\ultradefrag.log"
--
-- Example using environment variable:
--  Uses the temporary directory of the executing user
--    log_file_path = os.getenv("TEMP") .. "\\UltraDefrag_Logs\\ultradefrag.log"
--
-- Unicode characters cannot be included in log file paths.
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
-- Set show_menu_icons parameter to 1 to show menu icons
-- on Vista and more recent Windows editions.
-- Note: restart the program after this parameter adjustment.
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

show_menu_icons = $show_menu_icons

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- Set show_taskbar_icon_overlay parameter to 1 to show the taskbar icon
-- overlay indicating that the job is running on Windows 7 and more recent
-- Windows editions.
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

show_taskbar_icon_overlay = $show_taskbar_icon_overlay

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
-- Background color, in RGB format; default value is (255;255;255),
-- all color components should be in range 0-255,
-- (0;0;0) means black; (255;255;255) - white.
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

free_color_r = $free_color_r
free_color_g = $free_color_g
free_color_b = $free_color_b

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- this number helps to upgrade configuration file correctly, don't change it
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

version = $current_version

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- this code concatenates the filter variables, don't modify it
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
-- this code initializes the environment for UltraDefrag, don't modify it
-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

os.setenv("UD_IN_FILTER",in_filter)
os.setenv("UD_EX_FILTER",ex_filter)
os.setenv("UD_FILE_SIZE_THRESHOLD",file_size_threshold)
os.setenv("UD_FRAGMENTS_THRESHOLD",fragments_threshold)
os.setenv("UD_FRAGMENTATION_THRESHOLD",fragmentation_threshold)
os.setenv("UD_TIME_LIMIT",time_limit)
os.setenv("UD_REFRESH_INTERVAL",refresh_interval)
os.setenv("UD_DISABLE_REPORTS",disable_reports)
os.setenv("UD_DBGPRINT_LEVEL",dbgprint_level)
os.setenv("UD_LOG_FILE_PATH",log_file_path)
os.setenv("UD_DRY_RUN",dry_run)

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
-- version numbers 0-99 are reserved for 5.0.x series of the program
current_version = 13
old_version = 0
upgrade_needed = 1

-- parse command line
instdir = arg[1]
assert(instdir, "upgrade-guiopts.lua: the first argument is missing")

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
sizelimit = ""
fragments_threshold = 0
fragmentation_threshold = 0
time_limit = ""
refresh_interval = 100
disable_reports = 0
dbgprint_level = ""
log_file_path = ".\\logs\\ultradefrag.log"
dry_run = 0
seconds_for_shutdown_rejection = 60
disable_latest_version_check = 0
scale_by_dpi = 1
restore_default_window_size = 0
show_menu_icons = 1
show_taskbar_icon_overlay = 1
map_block_size = 4
grid_line_width = 1
grid_color_r = 0
grid_color_g = 0
grid_color_b = 0
free_color_r = 255
free_color_g = 255
free_color_b = 255

-- get user preferences
path = instdir .. "\\options\\guiopts.lua"
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
    f = io.open(path, "r")
    if f then
        contents = f:read("*all")
        f:close()
        f = assert(io.open(path .. ".old", "w"))
        f:write(contents)
        f:close()
    end

    -- RULES OF UPGRADE TO THE CURRENT VERSION
    if old_version == 0 then
        -- upgrade filters, now they should include wildcards
        in_filter = ""
        ex_filter = "*system volume information*;*temp*;*tmp*;*recycle*;*dllcache*;*ServicePackFiles*"
    end
    if not file_size_threshold then
        -- sizelimit has been superseded by file_size_threshold
        file_size_threshold = sizelimit
    end
    if old_version < 8 and log_file_path == "" then
        -- default log is needed for easier bug reporting
        log_file_path = ".\\logs\\ultradefrag.log"
    end
    if orig_in_filter then in_filter = orig_in_filter end
    if orig_ex_filter then ex_filter = orig_ex_filter end

    -- save the upgraded configuration
    f = assert(io.open(path, "w"))
    save_preferences(f)
    f:close()

    -- save guiopts-internals.lua when needed
    if old_version == 0 then
        if rx then
            f = assert(io.open(instdir .. "\\options\\guiopts-internals.lua", "w"))
            save_internal_preferences(f)
            f:close()
        end
    end
end
