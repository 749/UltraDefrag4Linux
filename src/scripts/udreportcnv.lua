#!/usr/local/bin/lua
--[[
  udreportcnv.lua - UltraDefrag report converter.
  Converts lua reports to HTML and other formats.
  Copyright (c) 2008-2012 by Dmitri Arkhangelski (dmitriar@gmail.com).

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

usage = [[
USAGE: lua udreportcnv.lua {path to Lua Report} {UltraDefrag installation directory} [-v]
]]

report_path = arg[1]
instdir     = arg[2]

assert(report_path, usage)
assert(instdir, usage)

webpage_name = ""

-------------------------------------------------------------------------------
-- Ancillary Procedures
-------------------------------------------------------------------------------

function write_unicode_character(f,c)
    local b1, b2, b3
    
    --- we'll convert a character to UTF-8 encoding
    if c < 0x80 then
        f:write(string.char(math.band(c,0xFF)))
        return
    end
    if c < 0x800 then -- 0x80 - 0x7FF: 2 bytes
        b2 = math.bor(0x80,math.band(c,0x3F))
        c = math.rshift(c,6)
        b1 = math.bor(0xC0,c)
        f:write(string.char(b1))
        f:write(string.char(b2))
        return
    end
    -- 0x800 - 0xFFFF: 3 bytes
    b3 = math.bor(0x80,math.band(c,0x3F))
    c = math.rshift(c,6)
    b2 = math.bor(0x80,math.band(c,0x3F))
    c = math.rshift(c,6)
    b1 = math.bor(0xE0,c)
    f:write(string.char(b1))
    f:write(string.char(b2))
    f:write(string.char(b3))
    return
end

function write_unicode_name_splitted(f,name)
    local name_length = table.maxn(name)
    local parts = {}
    local n_parts = 1
    local index = 1
    local part_length
    local chars_to_write
    
    -- write short names directly
    if name_length <= max_chars_per_line or max_chars_per_line == 0 or max_chars_per_line == nil then
        for j, b in ipairs(name) do
            write_unicode_character(f,b)
        end
        return
    end
    
    -- split a name to parts
    parts[1] = {}
    for j, b in ipairs(name) do
        parts[n_parts][index] = b
        if b == 0x5C then -- \ character
            n_parts = n_parts + 1
            parts[n_parts] = {}
            index = 1
        else
            index = index + 1
        end
    end
    
    chars_to_write = max_chars_per_line
    for j, part in pairs(parts) do
        part_len = table.maxn(part)
        if part_len == 0 then return end
        if part_len > chars_to_write then
            if j ~= 1 then
                f:write("<br>")
                chars_to_write = max_chars_per_line
            end
        end
        if part_len <= chars_to_write then
            for k, b in ipairs(part) do
                write_unicode_character(f,b)
            end
            chars_to_write = chars_to_write - part_len
        else -- current part is too long
            for k, b in ipairs(part) do
                write_unicode_character(f,b)
                chars_to_write = chars_to_write - 1
                if chars_to_write == 0 then
                    f:write("<br>")
                    chars_to_write = max_chars_per_line
                end
            end
        end
    end
end

function write_unicode_name(f,name)
    if split_long_names == 1 then
        write_unicode_name_splitted(f,name)
    else
        for j, b in ipairs(name) do
            write_unicode_character(f,b)
        end
    end
end

function get_javascript()
    local js = "", f
    if(enable_sorting == 1) then
        -- read udsorting.js file contents
        f = assert(io.open(instdir .. "\\scripts\\udsorting.js", "r"))
        js = f:read("*all")
        f:close()
    end
    if js == nil or js == "" then
        js = "function init_sorting_engine(){}\nfunction sort_items(criteria){}\n"
    end
    return js
end

function get_css()
    local css = ""
    local custom_css = ""
    local f

    -- read udreport.css file contents
    f = assert(io.open(instdir .. "\\scripts\\udreport.css", "r"))
    css = f:read("*all")
    f:close()
    if css == nil then
        css = ""
    end

    -- read udreport-custom.css file contents
    f = io.open(instdir .. "\\scripts\\udreport-custom.css", "r")
    if f ~= nil then
        custom_css = f:read("*all")
        f:close()
        if custom_css == nil then
            custom_css = ""
        end
    end

    return (css .. custom_css)
end

-------------------------------------------------------------------------------
-- HTML Output Procedures
-------------------------------------------------------------------------------

links_x1 = [[
<table class="links_toolbar" width="100%"><tbody>
<tr>
<td class="left"><a href="http://ultradefrag.sourceforge.net">Visit our Homepage</a></td>
<td class="center"><a href="file:///
]]

links_x2 = [[
\options\udreportopts.lua">View report options</a></td>
<td class="right">
<a href="http://www.lua.org/">Powered by Lua</a>
</td>
</tr>
</tbody></table>

]]

table_header = [[
<tr>
<td class="c"><a href="javascript:sort_items('fragments')">fragments</a></td>
<td class="c"><a href="javascript:sort_items('size')">size</a></td>
<td class="c"><a href="javascript:sort_items('name')">filename</a></td>
<td class="c"><a href="javascript:sort_items('comment')">comment</a></td>
<td class="c"><a href="javascript:sort_items('status')">status</a></td>
</tr>
]]

function write_web_page_header(f,js,css)
    local links_toolbar = links_x1 .. instdir .. links_x2
    local formatted_time = ""
    
    -- format time appropriate for locale
    if current_time ~= nil then
        formatted_time = os.date("%c",os.time(current_time))
    end
    
    f:write("<html>\n",
        "<head>\n",
            "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=UTF-8\">\n",
            "<title>Fragmented files on ", volume_letter, ": [", formatted_time, "]</title>\n",
            "<style type=\"text/css\">\n", css, "</style>\n",
            "<script language=\"javascript\">\n", js, "</script>\n",
        "</head>\n",
        "<body>\n",
            "<h3 class=\"title\">Fragmented files on ", volume_letter, ": (", formatted_time, ")</h3>\n",
            links_toolbar,
            "<div id=\"for_msie\">\n",
                "<table id=\"main_table\" border=\"1\" cellspacing=\"0\" width=\"100%\">\n"
    )
end

function write_web_page_footer(f)
    local links_toolbar = links_x1 .. instdir .. links_x2

    f:write("</table>\n",
        "</div>\n",
        links_toolbar,
        "<script type=\"text/javascript\">init_sorting_engine();</script>\n",
        "</body></html>\n"
    )
end

function write_main_table_body(f)
    for i, file in ipairs(files) do
        local class
        if file.filtered == 1 then class = "f" else class = "u" end
        f:write("<tr class=\"", class, "\"><td class=\"c\">", file.fragments,"</td>")
        f:write("<td class=\"filesize\" id=\"", file.size, "\">", file.hrsize,"</td><td>")
        write_unicode_name(f,file.uname)
        f:write("</td><td class=\"c\">", file.comment, "</td><td class=\"file-status\">", file.status, "</td></tr>\n")
    end
end

function build_web_page()
    local filename
    local pos = 0
    local js, css

    repeat
        pos = string.find(report_path,"\\",pos + 1,true)
        if pos == nil then filename = "fraglist.html" ; break end
    until string.find(report_path,"\\",pos + 1,true) == nil
    filename = string.sub(report_path,1,pos) .. "fraglist.html"

    -- note that 'b' flag is needed for utf-16 files
    local f = assert(io.open(filename,"wb"))

    -- get JavaScript and CSS
    js = get_javascript()
    css = get_css()
    
    -- write a web page header
    write_web_page_header(f,js,css)
    
    -- write a main table
    f:write(table_header)
    write_main_table_body(f)
    
    -- write a web page footer
    write_web_page_footer(f)

    f:close()
    return filename
end

function display_web_page(name)
    if os.shellexec ~= nil then
        os.shellexec(name,"open")
    else
        os.execute("cmd.exe /C " .. name)
    end
end

-------------------------------------------------------------------------------
-- Main Code
-------------------------------------------------------------------------------
-- read default options
dofile(instdir .. "\\options\\udreportopts.lua")

-- read custom (user defined) options
custom_options = instdir .. "\\options\\udreportopts-custom.lua"
f = io.open(custom_options)
if f ~= nil then
    f:close()
    dofile(custom_options)
end

-- read source file
dofile(report_path)

-- check the report format version
if format_version == nil or format_version < 4 then
    error("Reports produced by old versions of UltraDefrag are no more supported.\nUpdate the program at least to the 5.0.0 alpha3 version.")
end

-- build a web page containing a file fragmentation report
webpage_name = build_web_page()

-- display a web page if requested
if arg[3] == "-v" then
    display_web_page(webpage_name)
end
