#!/usr/local/bin/lua
--[[
  make-mingw-projects.lua - produces MinGW Developer Studio
  project files from *.build files.
  Copyright (c) 2013 Dmitri Arkhangelski (dmitriar@gmail.com).

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

--[[
  Synopsis: lua tools\make-mingw-projects.lua
  
  Note: run this script from /src directory as shown above.

  Notes for C programmers: 
    1. the first element of each array has index 1.
    2. only nil and false values are false, all other including 0 are true
--]]

-- global variables
debug_section = [[
[Debug]
// compiler 
workingDirectory=
arguments=
intermediateFilesDirectory=$output_dir
outputFilesDirectory=$output_dir
compilerPreprocessor=
extraCompilerOptions=
compilerIncludeDirectory=
noWarning=0
defaultWarning=0
allWarning=1
extraWarning=0
isoWarning=0
warningsAsErrors=0
debugType=1
debugLevel=2
exceptionEnabled=1
runtimeTypeEnabled=1
optimizeLevel=0

// linker
libraryPath=$lib_path
outputFilename=$out_filename
libraries=$libraries
extraLinkerOptions=$dbg_linker_opts
ignoreStartupFile=$ignoreStartupFile
ignoreDefaultLibs=$ignoreDefaultLibs
stripExecutableFile=0

// archive
extraArchiveOptions=

//resource
resourcePreprocessor=
resourceIncludeDirectory=
extraResourceOptions=

]]

release_section = [[
[Release]
// compiler 
workingDirectory=
arguments=
intermediateFilesDirectory=$output_dir
outputFilesDirectory=$output_dir
compilerPreprocessor=
extraCompilerOptions=
compilerIncludeDirectory=
noWarning=0
defaultWarning=0
allWarning=1
extraWarning=0
isoWarning=0
warningsAsErrors=0
debugType=0
debugLevel=1
exceptionEnabled=1
runtimeTypeEnabled=1
optimizeLevel=2

// linker
libraryPath=$lib_path
outputFilename=$out_filename
libraries=$libraries
extraLinkerOptions=$dbg_linker_opts
ignoreStartupFile=$ignoreStartupFile
ignoreDefaultLibs=$ignoreDefaultLibs
stripExecutableFile=0

// archive
extraArchiveOptions=

//resource
resourcePreprocessor=
resourceIncludeDirectory=
extraResourceOptions=

]]

-- subroutines
function expand (s)
  s = string.gsub(s, "$([%w_]+)", function (n)
        return tostring(_G[n])
      end)
  return s
end

function build_project_file(path)
    local f, line, files, search_path

    i, j, name = string.find(path,"^.*\\(.-)$")
    if not name then name = path end
    print(name .. " Preparing the project file generation...")

    name, target_type = "", ""
    libs, adlibs = {}, {}
    nativedll = 0
    mingw_project_rules = nil
    dofile(path)

    i, j, search_path = string.find(path,"^(.*)\\.-$")
    if not search_path then search_path = "" end
    if os.execute("cmd.exe /C dir /B " .. search_path .. "\\*.* >project_files") ~= 0 then
        error("Cannot get directory listing!")
    end
    f = assert(io.open("project_files","rt"))
    files = {}
    for line in f:lines() do
        table.insert(files,line)
    end
    f:close()
    os.execute("cmd.exe /C del /Q project_files")
    assert(files[1],"No project files found!")
    
    -- search for .def files
    deffile, mingw_deffile = "", ""
    for i, def in ipairs(files) do
        if string.find(def,"%.def$") then
            if string.find(def,"mingw%.def$") then
                mingw_deffile = def
            else
                deffile = def
            end
        end
    end
    if deffile == "" then deffile = mingw_deffile end
    if mingw_deffile == "" then mingw_deffile = deffile end

    if target_type == "console" or target_type == "gui" or target_type == "native" then
        target_ext = "exe"
    elseif target_type == "dll" then
        target_ext = "dll"
    elseif target_type == "driver" then
        target_ext = "sys"
    else
        error("Unknown target type: " .. target_type .. "!")
    end
    target_name = name .. "." .. target_ext

    f = assert(io.open(string.gsub(path,"%.build$","%.mdsp"),"wt"))

    f:write("[Project]\n")
    f:write("name=", name, "\n")
    if target_type ~= "dll" then
        f:write("type=0\n")
    else
        f:write("type=2\n")
    end
    if target_type == "console" or target_type == "gui" then
        f:write("defaultConfig=0\n\n") -- debug configuration
    else
        f:write("defaultConfig=1\n\n") -- release configuration
    end

    if target_type == "dll" then
        output_dir = "../../obj/" .. name
        lib_path = "..\\..\\lib"
        out_filename = "../../bin/" .. target_name
    else
        output_dir = "../obj/" .. name
        lib_path = "..\\lib"
        out_filename = "../bin/" .. target_name
    end
    libraries = ""
    rev_adlibs_libs = {}
    for i, v in ipairs(libs) do
        if i > 1 then libraries = libraries .. "," end
        libraries = libraries .. v
    end
    for i, v in ipairs(adlibs) do
        if libraries ~= "" then
            libraries = libraries .. ","
        end
        i, j, file = string.find(v,"^.*\\(.-)$")
        if not file then
            libraries = libraries .. v
            table.insert(rev_adlibs_libs,1,v)
        else
            libraries = libraries .. file
            table.insert(rev_adlibs_libs,1,file)
        end
    end
    -- include libraries in reverse order
    -- needed to link with static additional libraries
    for i, v in ipairs(rev_adlibs_libs) do
        if libraries ~= "" then
            libraries = libraries .. ","
        end
        libraries = libraries .. v
    end
    -- include standard libraries again
    -- needed to link with static additional libraries
    for i, v in ipairs(libs) do
        if libraries ~= "" then
            libraries = libraries .. ","
        end
        libraries = libraries .. v
    end
    dbg_linker_opts = ""
    if target_type == "gui" then
        dbg_linker_opts = "-mwindows"
    elseif target_type == "native" then
        dbg_linker_opts = "-Wl,--entry,_NtProcessStartup@4,--subsystem,native,--strip-all"
    elseif target_type == "dll" then
        if mingw_deffile ~= deffile then
            dbg_linker_opts = mingw_deffile .. " -Wl,--kill-at,--entry,_DllMain@12,--strip-all"
        else
            dbg_linker_opts = deffile .. " -Wl,--kill-at,--entry,_DllMain@12,--strip-all"
        end
    end
    ignoreStartupFile = 0
    ignoreDefaultLibs = 0
    if target_type == "native" or nativedll == 1 then
        ignoreStartupFile = 1
        ignoreDefaultLibs = 1
    end

    adsources = nil
    if mingw_project_rules then
        mingw_project_rules()
    end

    f:write(expand(debug_section))

    dbg_linker_opts = ""
    if target_type == "console" then
        dbg_linker_opts = "-Wl,--strip-all"
    elseif target_type == "gui" then
        dbg_linker_opts = "-mwindows -Wl,--strip-all"
    elseif target_type == "native" then
        dbg_linker_opts = "-Wl,--entry,_NtProcessStartup@4,--subsystem,native,--strip-all"
    elseif target_type == "dll" then
        if mingw_deffile ~= deffile then
            dbg_linker_opts = mingw_deffile .. " -Wl,--kill-at,--entry,_DllMain@12,--strip-all"
        else
            dbg_linker_opts = deffile .. " -Wl,--kill-at,--entry,_DllMain@12,--strip-all"
        end
    end
    f:write(expand(release_section))

    f:write("[Source]\n")
    index = 1
    for i, v in ipairs(files) do
        if string.find(v,"%.c(.-)$") then
            f:write(index, "=", v, "\n")
            index = index + 1
        end
    end
    if adsources then
        for i, v in ipairs(adsources) do
            f:write(index, "=", v, "\n")
            index = index + 1
        end
    end

    f:write("[Header]\n")
    index = 1
    for i, v in ipairs(files) do
        if string.find(v,"%.h$") then
            f:write(index, "=", v, "\n")
            index = index + 1
        end
    end

    f:write("[Resource]\n")
    index = 1
    for i, v in ipairs(files) do
        if string.find(v,"%.rc$") then
            f:write(index, "=", v, "\n")
            index = index + 1
        end
    end

    f:write("[Other]\n")
    index = 1
    if deffile ~= "" then
        f:write(index, "=", deffile, "\n")
        index = index + 1
    end
    if mingw_deffile ~= deffile then
        f:write(index, "=", mingw_deffile, "\n")
    end

    f:write("[History]\n")
    f:close()

    print("Project file generation completed successfully.")
end

-- the main code
if os.execute("cmd.exe /C dir /S /B *.build >files") ~= 0 then
    error("Cannot find .build files!")
end
f = assert(io.open("files","rt"))
for line in f:lines() do
    if not string.find(line,"\\obj\\") then
        build_project_file(line)
    end
end
f:close()
os.execute("cmd.exe /C del /Q files")
