#!/usr/local/bin/lua
--[[
  adjust-locale-header.lua - adjusts header of an UltraDefrag locale.
  Copyright (c) 2018 Dmitri Arkhangelski (dmitriar@gmail.com).

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

-- Usage: lua adjust-locale-header.lua {path to a locale}

-- parse parameters
assert(arg[1],"File name must be specified!")

-- read the entire file
f = assert(io.open(arg[1], "r"))
contents = f:read("*all")
f:close()

-- adjust the header
new_contents = string.gsub(contents,
    "# SOME DESCRIPTIVE TITLE%.",
    "# UltraDefrag localization resources%."
)
new_contents = string.gsub(new_contents,
    "# Copyright %(C%) YEAR UltraDefrag Development Team",
    "# Copyright %(C%) 2018 UltraDefrag Development Team%."
)
new_contents = string.gsub(new_contents,
    "# This file is distributed under the same license as the PACKAGE package%.",
    "# This file is distributed under the Creative Commons Attribution 3%.0 License%."
)

-- save the updated contents
f = assert(io.open(arg[1], "w"))
f:write(new_contents)
f:close()
