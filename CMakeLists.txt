
cmake_minimum_required (VERSION 2.6)

project(lcmsWorld)

 set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/glfw )
include_directories(${GLFW_INCLUDE_DIRS})



if(NOT MSVC)

find_package(Threads)

find_package(X11 REQUIRED)

 
include_directories(${X11_INCLUDE_DIR})
add_definitions(-DGCC)
add_definitions(-D__LINUX__)
 


endif()


include_directories( ${PROJECT_SOURCE_DIR}/src/GL )
include_directories( ${PROJECT_SOURCE_DIR}/src )
include_directories( ${PROJECT_SOURCE_DIR}/src/curlinc )
include_directories( ${PROJECT_SOURCE_DIR}/src/libcurl )
file(GLOB SOURCES  
${PROJECT_SOURCE_DIR}/src/*.cpp 
${PROJECT_SOURCE_DIR}/src/*.c 
${PROJECT_SOURCE_DIR}/src/*.h 

${PROJECT_SOURCE_DIR}/src/imgui/*.cpp 
${PROJECT_SOURCE_DIR}/src/imgui/*.c 
${PROJECT_SOURCE_DIR}/src/imgui/*.h 

${PROJECT_SOURCE_DIR}/src/expat-2.2.0/*.cpp 
${PROJECT_SOURCE_DIR}/src/expat-2.2.0/*.c 
${PROJECT_SOURCE_DIR}/src/expat-2.2.0/*.h 


${PROJECT_SOURCE_DIR}/src/mzParser/*.cpp 
${PROJECT_SOURCE_DIR}/src/mzParser/*.c 
${PROJECT_SOURCE_DIR}/src/mzParser/*.h 


${PROJECT_SOURCE_DIR}/src/zlib/*.cpp 
${PROJECT_SOURCE_DIR}/src/zlib/*.c 
${PROJECT_SOURCE_DIR}/src/zlib/*.h 


${PROJECT_SOURCE_DIR}/src/imgui/*.cpp 
${PROJECT_SOURCE_DIR}/src/imgui/*.c 
${PROJECT_SOURCE_DIR}/src/imgui/*.h 

  




${PROJECT_SOURCE_DIR}/src/gl3w/*.c 
${PROJECT_SOURCE_DIR}/src/gl3w/*.h 
${PROJECT_SOURCE_DIR}/src/gl3w/KHR/*.h
 


${PROJECT_SOURCE_DIR}/src/glm/*.h 
${PROJECT_SOURCE_DIR}/src/curlinc/curl/*.h 

${PROJECT_SOURCE_DIR}/src/glmx/*.h 




)

 
 file(GLOB MSSOURCES  
 ${PROJECT_SOURCE_DIR}/src/glfw/src/context.c 
${PROJECT_SOURCE_DIR}/src/glfw/src/egl_context.c 
${PROJECT_SOURCE_DIR}/src/glfw/src/init.c 
  
${PROJECT_SOURCE_DIR}/src/glfw/src/input.c 
${PROJECT_SOURCE_DIR}/src/glfw/src/monitor.c 
${PROJECT_SOURCE_DIR}/src/glfw/src/osmesa_context.c 
${PROJECT_SOURCE_DIR}/src/glfw/src/vulkan.c 
${PROJECT_SOURCE_DIR}/src/glfw/src/wgl_context.c 
${PROJECT_SOURCE_DIR}/src/glfw/src/win32_init.c 
${PROJECT_SOURCE_DIR}/src/glfw/src/win32_joystick.c 
${PROJECT_SOURCE_DIR}/src/glfw/src/win32_monitor.c 
${PROJECT_SOURCE_DIR}/src/glfw/src/win32_platform.c 
${PROJECT_SOURCE_DIR}/src/glfw/src/win32_thread.c 
${PROJECT_SOURCE_DIR}/src/glfw/src/win32_time.c 
${PROJECT_SOURCE_DIR}/src/glfw/src/win32_window.c 
${PROJECT_SOURCE_DIR}/src/glfw/src/window.c 

${PROJECT_SOURCE_DIR}/src/libcurl/*.c 
${PROJECT_SOURCE_DIR}/src/libcurl/vauth/*.c 
${PROJECT_SOURCE_DIR}/src/libcurl/vtls/*.c 
)
 
add_definitions(-D_GLFW_USE_CONFIG_H)

add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE) 
add_definitions(-D_CRT_SECURE_NO_WARNINGS) 
add_definitions(-DCURL_STATICLIB) 
add_definitions(-fpermissive)
add_definitions(-DFINAL)

add_definitions(-D_FILE_OFFSET_BITS=64)
 
if(MSVC)
add_executable(lcmsWorld ${SOURCES} ${MSSOURCES} )
else()
add_executable(lcmsWorld ${SOURCES}  )
endif()

 
set_target_properties(lcmsWorld PROPERTIES
            CXX_STANDARD 17 )

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()


            
if(MSVC)

   SET(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS}  /fp:fast")

set(CompilerFlags
    CMAKE_CXX_FLAGS
    CMAKE_CXX_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_RELEASE)
foreach(CompilerFlag ${CompilerFlags})
    message("before replace: " ${${CompilerFlag}})
    string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    
    message("after replace: " ${${CompilerFlag}})
endforeach()
 
 target_link_libraries(lcmsWorld "legacy_stdio_definitions.lib")
target_link_libraries(lcmsWorld "kernel32.lib")
target_link_libraries(lcmsWorld "user32.lib")
target_link_libraries(lcmsWorld "gdi32.lib")
target_link_libraries(lcmsWorld "winspool.lib")
target_link_libraries(lcmsWorld "comdlg32.lib")
target_link_libraries(lcmsWorld "advapi32.lib")
target_link_libraries(lcmsWorld "shell32.lib")
target_link_libraries(lcmsWorld "ole32.lib")
target_link_libraries(lcmsWorld "oleaut32.lib")
target_link_libraries(lcmsWorld "uuid.lib")
target_link_libraries(lcmsWorld "odbc32.lib")
target_link_libraries(lcmsWorld "odbccp32.lib")
target_link_libraries(lcmsWorld "opengl32.lib")
target_link_libraries(lcmsWorld "wldap32.lib")
target_link_libraries(lcmsWorld "Crypt32.lib")
target_link_libraries(lcmsWorld "ws2_32.lib") 
target_link_libraries(lcmsWorld "Psapi.lib")
 
 
else()


set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

 
target_link_libraries(lcmsWorld ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(lcmsWorld  ${CMAKE_DL_LIBS})
target_link_libraries(lcmsWorld  ${X11_LIBRARIES})
target_link_libraries(lcmsWorld glfw ${GLFW_LIBRARIES})
endif()

