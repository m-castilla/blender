# ***** BEGIN GPL LICENSE BLOCK *****
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public Licenses
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
# ***** END GPL LICENSE BLOCK *****


# Build defmerger executable
# ---------------------------

if(WIN32)
  include_directories(
    ../blenlib
    ../../../intern/cycles
    ../../../intern/guardedalloc
    ../makesdna
    ../../../intern/utfconv
  )
endif()

set(SRC_EXEC 
  defmerger.cpp
)

  # for winstuff.cw and winstuff_dir.c. winstuff.c has a dependency on blender kernel from not needed code for a binary other than blender. 
  # With this we avoid such dependency. And it indicates that this is a binary that is not blender.
  add_definitions(-DUSE_STANDALONE)

if(WIN32)
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /nodefaultlib:MSVCRT.lib")



  list(APPEND SRC_EXEC
    ../blenlib/intern/winstuff.c
    ../blenlib/intern/winstuff_dir.c
    ../../../intern/utfconv/utfconv.c
  )
endif()

set(LIB_EXEC
  bf_defmerger
  bf_blenlib
  cycles_util
  ${ZLIB_LIBRARY}
  ${OPENIMAGEIO_LIBRARIES}
  ${PLATFORM_LINKLIBS}
  ${BOOST_LIBRARIES}
)

add_executable(defmerger ${SRC_EXEC})

target_link_libraries(defmerger ${LIB_EXEC})



