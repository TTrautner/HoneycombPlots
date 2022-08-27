file(GLOB_RECURSE molumes_sources *.cpp *.h)
file(GLOB imgui_sources ${CMAKE_SOURCE_DIR}/lib/imgui/*.cpp ${CMAKE_SOURCE_DIR}/lib/imgui/*.h)
file(GLOB lodepng_sources ${CMAKE_SOURCE_DIR}/lib/lodepng/lodepng.cpp ${CMAKE_SOURCE_DIR}/lib/lodepng/lodepng.h)

add_executable(molumes ${molumes_sources} ${imgui_sources} ${lodepng_sources})

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/lib/glm)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/lib/glbinding)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/lib/globjects)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/lib/glfw)
 
find_package(glm REQUIRED)
find_package(glbinding REQUIRED)
find_package(globjects REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Matlab COMPONENTS ENG_LIBRARY)
find_package(OpenMP)

include_directories(${CMAKE_SOURCE_DIR}/lib/HBAOPlus/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/imgui/)
include_directories(${CMAKE_SOURCE_DIR}/lib/lodepng/)

target_link_libraries(molumes PUBLIC glfw)
target_link_libraries(molumes PUBLIC glbinding::glbinding )
target_link_libraries(molumes PUBLIC glbinding::glbinding-aux )
target_link_libraries(molumes PUBLIC globjects::globjects)
if(OpenMP_CXX_FOUND)
    target_link_libraries(molumes PUBLIC OpenMP::OpenMP_CXX)
endif()


if (Matlab_FOUND)
	include_directories(${Matlab_INCLUDE_DIRS})
	target_link_libraries(molumes PUBLIC ${MATLAB_LIBRARIES})
	target_compile_definitions(molumes INTERFACE HAVE_MATLAB)
endif()

set_target_properties(molumes PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
