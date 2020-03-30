#!/usr/local/bin/lua
--[[
  udreportcnv.lua - UltraDefrag report converter.
  Converts lua reports to HTML and other formats.
  Copyright (c) 2008-2012 Dmitri Arkhangelski (dmitriar@gmail.com).

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

html_report_path = ""
text_report_path = ""

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

function display_report(path)
    if os.shellexec ~= nil then
        os.shellexec(path,"open")
    else
        os.execute("cmd.exe /C " .. path)
    end
end

-------------------------------------------------------------------------------
-- Internationalization Procedures
-------------------------------------------------------------------------------

localized = {}

function get_localization_strings()
    local f, i, j, lang = nil
    local contents
    local a, b, c, name, value, s
    local eq_sign_detected
    local value_length
    
    -- get selected language name
    f = io.open(instdir .. "\\lang.ini","r")
    if f == nil then return end
    for line in f:lines() do
        i, j, lang = string.find(line,"^%s*Selected%s*=%s*(.-)%s*$")
        if lang ~= nil then break end
    end
    f:close()
    if lang == nil then return end
    
    -- read .lng file as a whole
    f = io.open(instdir .. "\\i18n\\" .. lang .. ".lng","rb")
    if f == nil then return end
    contents = f:read("*all")
    f:close()
    if contents == nil then return end
    
    -- fill table of localized strings
    name = ""; value = {}
    value_length = 0
    eq_sign_detected = 0
    for c1, c2 in string.gfind(contents,"(.)(.)") do
        -- decode subsequent UTF-16 LE character
        a = string.byte(c1)
        b = math.lshift(string.byte(c2),8)
        c = math.bor(a,b)
        if c == 0xD or c == 0xA then
            -- \r or \n detected
            if name ~= "" and value[1] ~= nil then
                -- remove trailing white space from the name
                i, j, s = string.find(name,"^%s*(.-)%s*$")
                if s ~= nil then name = s else name = "" end
                -- remove trailing white space from the value
                for i = value_length, 1, -1 do
                    if value[i] == 0x20 or value[i] == 0x9 then
                        value[i] = nil
                    else
                        break
                    end
                end
                if name ~= "" and value[1] ~= nil then
                    -- add localized string to the table
                    localized[name] = value
                end
            end
            -- reset both name and value
            name = ""; value = {}
            value_length = 0
            eq_sign_detected = 0
        elseif c == 0x3D then
            eq_sign_detected = 1
        else
            if eq_sign_detected == 1 then
                if c == 0x20 or c == 0x9 then
                    -- white space detected
                    if value[1] ~= nil then
                        table.insert(value,c)
                        value_length = value_length + 1
                    end
                else
                    table.insert(value,c)
                    value_length = value_length + 1
                end
            else
                name = string.format("%s%c",name,c)
            end
        end
    end
end

function write_localized_string(f,key,default_string)
    if localized[key] ~= nil then
        for i, c in ipairs(localized[key]) do
            write_unicode_character(f,c)
        end
    else
        f:write(default_string)
    end
end

-------------------------------------------------------------------------------
-- Plain Text Output Procedures
-------------------------------------------------------------------------------

-- Plain text reports may be used in batch scripts to extract
-- information from. So, let's avoid their localization.

function write_text_header(f)
    local formatted_time = ""
    
    -- format time appropriate for locale
    if current_time ~= nil then
        formatted_time = os.date("%c",os.time(current_time))
    end
    
    -- write byte order mark
    f:write(string.char(0xEF))
    f:write(string.char(0xBB))
    f:write(string.char(0xBF))

    f:write(";---------------------------------------------------------------------------------------------\r\n")
    f:write("; Fragmented files on ", volume_letter, ": [", formatted_time, "]\r\n;\r\n")
    f:write("; Fragments    Filesize  Comment      Status    Filename\r\n")
    f:write(";---------------------------------------------------------------------------------------------\r\n")
    f:write("\r\n")
end

function write_main_table(f)
    for i, file in ipairs(files) do
        if file.filtered == 0 then
            f:write(string.format("%11u%12s%9s%12s    ", file.fragments, 
                string.gsub(file.hrsize,"&nbsp;"," "), file.comment,
                file.status)
            )
            for j, b in ipairs(file.uname) do
                write_unicode_character(f,b)
            end
            f:write("\r\n")
        end
    end
end

function build_text_report()
    local filename
    local pos = 0

    repeat
        pos = string.find(report_path,"\\",pos + 1,true)
        if pos == nil then filename = "fraglist.txt" ; break end
    until string.find(report_path,"\\",pos + 1,true) == nil
    filename = string.sub(report_path,1,pos) .. "fraglist.txt"

    -- note that 'b' flag is needed for utf-16 files
    local f = assert(io.open(filename,"wb"))

    -- write the header
    write_text_header(f)
    
    -- write the main table
    write_main_table(f)

    f:close()
    return filename
end

-------------------------------------------------------------------------------
-- HTML Output Procedures
-------------------------------------------------------------------------------

-- HTML reports are intended to be opened in a web
-- browser. So, let's use localized strings there.

links_1 = [[
<table class="links_toolbar" width="100%"><tbody>
<tr>
<td class="left"><a href="http://ultradefrag.sourceforge.net">
]]

links_2 = [[
</a></td>
<td class="center"><a href="file:///
]]

links_3 = [[
\options\udreportopts.lua">
]]

links_4 = [[
</a></td>
<td class="right">
<a href="http://www.lua.org/">
]]

links_5 = [[
</a>
</td>
</tr>
</tbody></table>

]]

-- these markups must be identical, except of representation
table_head        = [[<table id="main_table" border="1" cellspacing="0" width="100%">]]
table_head_for_js = [[<table id=\"main_table\" border=\"1\" cellspacing=\"0\" width=\"100%%\">]]

function write_links_toolbar(f)
    f:write(links_1)
    write_localized_string(f,"VISIT_HOMEPAGE","Visit our Homepage")
    f:write(links_2, instdir, links_3)
    write_localized_string(f,"VIEW_REPORT_OPTIONS","View report options")
    f:write(links_4)
    write_localized_string(f,"POWERED_BY_LUA","Powered by Lua")
    f:write(links_5)
end

function write_page_title(f)
    write_localized_string(f,"FRAGMENTED_FILES_ON","Fragmented files on")
end

function write_main_table_header(f)
    f:write("<tr>\n<td class=\"c\"><a href=\"javascript:sort_items(\'fragments\')\">")
    write_localized_string(f,"FRAGMENTS","fragments")
    f:write("</a></td>\n<td class=\"c\"><a href=\"javascript:sort_items(\'size\')\">")
    write_localized_string(f,"SIZE","size")
    f:write("</a></td>\n<td class=\"c\"><a href=\"javascript:sort_items(\'name\')\">")
    write_localized_string(f,"FILENAME","filename")
    f:write("</a></td>\n<td class=\"c\"><a href=\"javascript:sort_items(\'comment\')\">")
    write_localized_string(f,"COMMENT","comment")
    f:write("</a></td>\n<td class=\"c\"><a href=\"javascript:sort_items(\'status\')\">")
    write_localized_string(f,"STATUS","status")
    f:write("</a></td>\n</tr>\n")
end

function write_file_status(f,file)
    if file.status == "locked" then
        write_localized_string(f,"LOCKED","locked")
    elseif file.status == "move failed" then
        write_localized_string(f,"MOVE_FAILED","move failed")
    elseif file.status == "invalid" then
        write_localized_string(f,"INVALID","invalid")
    else
        f:write(file.status)
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

    -- replace $TABLE_HEAD by actual markup
    return string.gsub(js,"$TABLE_HEAD",table_head_for_js)
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

function write_web_page_header(f,js,css)
    local formatted_time = ""
    
    -- format time appropriate for locale
    if current_time ~= nil then
        formatted_time = os.date("%c",os.time(current_time))
    end
    
    f:write(
        "<html>\n",
         "<head>\n",
          "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=UTF-8\">\n",
          "<title>"
    )
    write_page_title(f)
    f:write(" ", volume_letter, ": [", formatted_time, "]</title>\n",
         "<style type=\"text/css\">\n", css, "</style>\n",
         "<script language=\"javascript\">\n", js, "</script>\n",
        "</head>\n",
        "<body>\n",
         "<h3 class=\"title\">"
    )
    write_page_title(f)
    f:write(" ", volume_letter, ": (", formatted_time, ")</h3>\n")
    write_links_toolbar(f)
    f:write("<div id=\"for_msie\">\n",table_head,"\n")
end

function write_web_page_footer(f)
    f:write("</table>\n</div>\n")
    write_links_toolbar(f)
    f:write("<script type=\"text/javascript\">init_sorting_engine();</script>\n</body></html>\n")
end

function write_main_table_body(f)
    for i, file in ipairs(files) do
        local class
        if file.filtered == 1 then class = "f" else class = "u" end
        f:write("<tr class=\"", class, "\"><td class=\"c\">", file.fragments,"</td>")
        f:write("<td class=\"filesize\" id=\"", file.size, "\">", file.hrsize,"</td><td>")
        write_unicode_name(f,file.uname)
        f:write("</td><td class=\"c\">", file.comment, "</td><td class=\"file-status\">")
        write_file_status(f,file)
        f:write("</td></tr>\n")
    end
end

function build_html_report()
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
    
    -- write the web page header
    write_web_page_header(f,js,css)
    
    -- write the main table
    write_main_table_header(f)
    write_main_table_body(f)
    
    -- write the web page footer
    write_web_page_footer(f)

    f:close()
    return filename
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
if format_version == nil or format_version < 5 then
    error("Reports produced by old versions of UltraDefrag are no more supported.\nUpdate the program at least to the 5.0.3 version.")
end

-- read i18n strings
get_localization_strings()

-- build file fragmentation reports
if produce_html_report == 1 then
    html_report_path = build_html_report()
end
if produce_plain_text_report == 1 then
    text_report_path = build_text_report()
end

-- display report if requested
if arg[3] == "-v" then
    if produce_html_report == 1 then
        display_report(html_report_path)
    elseif produce_plain_text_report == 1 then
        display_report(text_report_path)
    end
end
