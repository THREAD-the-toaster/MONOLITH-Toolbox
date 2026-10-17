cmake_minimum_required(VERSION 3.21)
project(MONOLITH_Toolbox VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_language(RC)
configure_file(src/version.rc.in "${CMAKE_CURRENT_BINARY_DIR}/version.rc" @ONLY)
add_executable(MONOLITH_Toolbox WIN32 src/main.cpp "${CMAKE_CURRENT_BINARY_DIR}/version.rc")
# The official MSVC release package should not require extra VC runtime DLLs.
if(MSVC)
  set_property(TARGET MONOLITH_Toolbox PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
target_compile_definitions(MONOLITH_Toolbox PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
# MinGW requires Unicode startup to resolve wWinMain instead of WinMain.
if(MINGW)
  target_link_options(MONOLITH_Toolbox PRIVATE -municode)
endif()
target_link_libraries(MONOLITH_Toolbox PRIVATE d2d1 dwrite dwmapi shell32 ole32 windowscodecs advapi32 uuid)
if(MSVC)
 target_compile_options(MONOLITH_Toolbox PRIVATE /W4 /permissive- /utf-8)
else()
 target_compile_options(MONOLITH_Toolbox PRIVATE -Wall -Wextra)
endif()

# Keep the MinGW build runnable from Explorer as well as from CLion.
if(MINGW)
  get_filename_component(MONOLITH_MINGW_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
  foreach(runtime libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
    if(EXISTS "${MONOLITH_MINGW_BIN}/${runtime}")
      add_custom_command(TARGET MONOLITH_Toolbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${MONOLITH_MINGW_BIN}/${runtime}" "$<TARGET_FILE_DIR:MONOLITH_Toolbox>"
        VERBATIM)
    endif()
  endforeach()
endif()



# Ship the original logo and SVG sources with their WIC-compatible renditions.
add_custom_command(TARGET MONOLITH_Toolbox POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:MONOLITH_Toolbox>/assets"
  VERBATIM)

