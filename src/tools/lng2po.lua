#!/usr/local/bin/lua
--[[
  lng2po.lua - Converts translations from LNG to PO.
  Copyright (c) 2013 UltraDefrag Development Team.

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
--   2. only nil and false values are false, all others, including 0, are true

-- USAGE: lua lng2po.lua

--[[ debugging:
      0 ... off
      1 ... on
      2 ... all
--]]

debugging = 0

-- set default file paths
lng_folder = "..\\..\\..\\release\\src\\gui\\i18n\\"
lng_template_file = lng_folder .. "translation.template"

tx_folder = ".\\transifex\\"
po_folder = tx_folder .. "\\translations\\"

-- language pairs: language name, ISO code
language_pairs = {
  { "Albanian", "sq" } ,
  { "Arabic", "ar" } ,
  { "Armenian", "hy" } ,
  { "Belarusian", "be" } ,
  { "Bengali", "bn" } ,
  { "Bosnian", "bs" } ,
  { "Bulgarian", "bg" } ,
  { "Burmese (Padauk)", "my" } ,
  { "Catalan", "ca" } ,
  { "Chinese (Simplified)", "zh_CN" } ,
  { "Chinese (Traditional)", "zh_TW" } ,
  { "Croatian", "hr" } ,
  { "Czech", "cs" } ,
  { "Danish", "da" } ,
  { "Dutch", "nl" } ,
  { "English (GB)", "en_GB" } ,
  { "Estonian", "et" } ,
  { "Farsi", "fa" } ,
  { "Filipino (Tagalog)", "tl" } ,
  { "Finnish", "fi" } ,
  { "French", "fr" } ,
  { "Galician", "gl" } ,
  { "Georgian", "ka" } ,
  { "German", "de" } ,
  { "Greek", "el" } ,
  { "Hebrew", "he" } ,
  { "Hindi", "hi" } ,
  { "Hungarian", "hu" } ,
  { "Icelandic", "is" } ,
  { "Iloko", "ilo" } ,
  { "Indonesian (Bahasa Indonesia)", "id" } ,
  { "Italian", "it" } ,
  { "Japanese", "ja" } ,
  { "Javanese", "jv" } ,
  { "Kapampangan", "pam" } ,
  { "Korean", "ko" } ,
  { "Latin", "la" } ,
  { "Latvian", "lv" } ,
  { "Lithuanian", "lt" } ,
  { "Macedonian", "mk" } ,
  { "Malay", "ms" } ,
  { "Norwegian", "no" } ,
  { "Polish", "pl" } ,
  { "Portuguese (BR)", "pt_BR" } ,
  { "Portuguese", "pt" } ,
  { "Romanian", "ro" } ,
  { "Russian", "ru" } ,
  { "Serbian", "sr" } ,
  { "Slovak", "sk" } ,
  { "Slovenian", "sl" } ,
  { "Spanish (AR)", "es_AR" } ,
  { "Spanish (ES)", "es" } ,
  { "Spanish (MEX)", "es_MX" } ,
  { "Swedish", "sv" } ,
  { "Tamil", "ta" } ,
  { "Thai", "th" } ,
  { "Turkish", "tr" } ,
  { "Ukrainian", "uk" } ,
  { "Vietnamese", "vi" } ,
  { "Waray-Waray", "war" } ,
  { "Yiddish", "yi" }
}

translation_pairs = {}

po_header = [[
# SOME DESCRIPTIVE TITLE.
# Copyright (C) YEAR UltraDefrag Development Team
# This file is distributed under the same license as the PACKAGE package.
# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.
#
#, fuzzy
msgid ""
msgstr ""
"Project-Id-Version: PACKAGE VERSION\n"
"Report-Msgid-Bugs-To: http://sourceforge.net/p/ultradefrag/bugs/\n"
"POT-Creation-Date: YEAR-MO-DA HO:MI+ZONE\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\n"
"Language-Team: LANGUAGE <LL@li.org>\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"
]]

if debugging > 0 then print(po_header) end

-- create table with English translations
print("Collecting translations from template ...\n")

f = assert(io.open(lng_template_file, "r"))
  for line in f:lines() do
    local option, value

    i, j, option, value = string.find(line,"^%s*(.-)%s*=%s*(.-)%s*$")

    if option ~= nil then
      value = string.gsub(value, "^Sort by", "By")
      value = string.gsub(value, "^Sort in", "In")
      value = string.gsub(value, "Ultra Defragmenter", "UltraDefrag")
      value = string.gsub(value, "Ultra Defrag", "UltraDefrag")
      value = string.gsub(value, "^Please Confirm", "Please confirm")
      value = string.gsub(value, "Space", "space")
      value = string.gsub(value, "^seconds until", "%%lu seconds until")
      value = string.gsub(value, "^A job", "The job")

      table.insert(translation_pairs, option .. "=" .. value)

      if debugging > 0 then print(option .. "=" .. value) end
    end
  end
f:close()

-- process language files
print("Converting translations from LNG to PO ...\n")

for i, v in ipairs(language_pairs) do
  lng_file = lng_folder .. v[1] .. ".lng"
  po_file  = po_folder  .. v[2] .. ".pot"

  if debugging > 1 then print( string.format("%s ... %s", lng_file, po_file)) end

  fo = assert(io.open(po_file, "w"))
    fo:write(po_header)
    index = 1

    fi = assert(io.open(lng_file, "r"))
      for line in fi:lines() do
        local option, value, i, j

        i, j, option, value = string.find(line,"^%s*(.-)%s*=%s*(.-)%s*$")

        if option ~= nil then
          if option ~= "WHEN_DONE_EXIT" then
            value = string.gsub(value, "\"", "")
            value = string.gsub(value, "Ultra Defragmenter", "UltraDefrag")
            value = string.gsub(value, "Ultra Defrag", "UltraDefrag")
            value = string.gsub(value, "^A job", "The job")

            template_str = string.match(translation_pairs[index], "^.+=(.+)$")

            fo:write("\n")
            fo:write("msgid \"" .. template_str .. "\"\n")

            template_filtered = string.lower(string.gsub(template_str, "&", ""))
            value_filtered    = string.lower(string.gsub(value,        "&", ""))

            if template_filtered ~= value_filtered and
                string.match(value, "^Sort by") == nil and
                string.match(value, "^Sort in") == nil and
                string.match(value, "FAQ") == nil then

              if string.match(option, "^SECONDS_TILL_") ~= nil then value = "%lu " .. value end

              fo:write("msgstr \"" .. value .. "\"\n")
            else
              fo:write("msgstr \"\"\n")
            end
          end

          index = index + 1
        end
      end
    fi:close()
  fo:close()

  print( string.format( "%2d ... %-29s ... %-5s ... DONE", i, v[1], v[2] ))
end

print("\nFinished ...\n")
