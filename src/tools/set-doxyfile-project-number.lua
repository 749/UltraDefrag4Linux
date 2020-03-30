#!/usr/local/bin/lua
--[[
  set-doxyfile-project-number.lua - Sets PROJECT_NUMBER field in doxyfile.
  Copyright (c) 2010-2013 Dmitri Arkhangelski (dmitriar@gmail.com).

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

-- NOTES for C programmers: 
--   1. the first element of each array has index 1.
--   2. only nil and false values are false, all other including 0 are true

-- USAGE: lua set-doxyfile-project-number.lua <filename> <project_number>

-- parse parameters
assert(arg[1],"File name must be specified!")
assert(arg[2],"Poject number must be specified!")

-- read the entire file
f = assert(io.open(arg[1], "r"))
contents = f:read("*all")
f:close()

-- set the project number
new_contents = string.gsub(contents,"PROJECT_NUMBER%s*=%s*%d+.%d+.%d+","PROJECT_NUMBER         = " .. arg[2])

-- save the updated contents
f = assert(io.open(arg[1], "w"))
f:write(new_contents)
f:close()
