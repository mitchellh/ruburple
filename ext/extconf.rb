# Ruburple - a ruby wrapper for libpurple
# Copyright (C) 2007  Martin Kihlgren <zond at troja dot ath dot cx>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

require 'mkmf'

def crash(s)
  puts "--------------------------------------------------"
  puts " extconf failure: #{s}"
  puts "--------------------------------------------------"
  exit 1
end

unless find_executable("pkg-config")
  crash("pkg-config needed")
end

$CFLAGS += " -g -gdwarf-2 -g3 " + `pkg-config --cflags glib-2.0 gthread-2.0`.strip
$LIBS += " " + `pkg-config --libs glib-2.0 gthread-2.0`.strip

unless have_library('glib-2.0')
  crash "libglib-2.0 needed"
end

unless have_library('gthread-2.0')
  crash "gthread-2.0 needed"
end

unless find_header("libpurple/core.h", "/usr/include", "/usr/local/include", "/opt/local/include")
  crash "libpurple needed"
end

unless find_library("purple", "purple_core_init", "/usr/local/lib", "/opt/local/lib")
  crash "libpurple needed"
end

create_makefile("ruburple_ext")
