cmake_minimum_required(VERSION 3.21)

project(PageTurnSkillBooks VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# CommonLibSSE-NG jako podprojekt (sklonowany do extern/ - patrz .github/workflows/build.yml)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/CommonLibSSE-NG CommonLibSSE EXCLUDE_FROM_ALL)
include(extern/CommonLibSSE-NG/cmake/CommonLibSSE.cmake)

find_package(minhook CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_commonlibsse_plugin(${PROJECT_NAME}
	AUTHOR "mogar88"
	VERSION ${PROJECT_VERSION}
	USE_ADDRESS_LIBRARY
	SOURCES src/main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE src)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)
target_link_libraries(${PROJECT_NAME} PRIVATE minhook::minhook spdlog::spdlog)
