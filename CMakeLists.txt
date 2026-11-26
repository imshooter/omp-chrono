cmake_minimum_required(VERSION 3.19)
project(Chrono VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)

FetchContent_Declare(omp-sdk
	GIT_REPOSITORY https://github.com/openmultiplayer/open.mp-sdk.git
	GIT_TAG 08cc69a32d96c0eb8fe063d26c78259e54ca853f
)
FetchContent_Declare(pawn
	GIT_REPOSITORY https://github.com/openmultiplayer/compiler.git
	GIT_TAG 29f1a8c7fd2e01929807cd8c50353fbc93bbd651
	SOURCE_SUBDIR none
)
FetchContent_MakeAvailable(omp-sdk pawn)

add_library(Chrono SHARED
	src/main.cpp
	src/natives.cpp
	src/zones.cpp
	src/format.cpp
	src/parse.cpp
)
set_target_properties(Chrono PROPERTIES PREFIX "")
target_include_directories(Chrono PRIVATE
	"${pawn_SOURCE_DIR}/source"
	"${pawn_SOURCE_DIR}/source/linux"
	src
)
target_compile_definitions(Chrono PRIVATE
	HAVE_STDINT_H=1
	PAWN_CELL_SIZE=32
	COMPONENT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
	COMPONENT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
	COMPONENT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
	COMPONENT_VERSION="${PROJECT_VERSION}"
)
if(WIN32)
	target_compile_definitions(Chrono PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()
if(MSVC)
	target_compile_options(Chrono PRIVATE /W3 /utf-8 /O2)
else()
	target_compile_options(Chrono PRIVATE -Wall -fvisibility=hidden -fvisibility-inlines-hidden)
	target_link_options(Chrono PRIVATE -static-libstdc++ -static-libgcc -Wl,--exclude-libs,ALL)
endif()
target_link_libraries(Chrono PRIVATE OMP-SDK)
