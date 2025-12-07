cmake_minimum_required( VERSION 3.28 FATAL_ERROR )

project( mutex
	VERSION 0.1.0
	DESCRIPTION ""
	LANGUAGES CXX
)

set( HEADERS
	include/mutex/recursive_shared_mutex.h
)

set( SOURCES
	src/recursive_shared_mutex.cpp
)

add_library( ${PROJECT_NAME} STATIC
	${HEADERS}
	${SOURCES}
)

target_compile_features( ${PROJECT_NAME} 
	PUBLIC 
		cxx_std_17 
)

target_include_directories( ${PROJECT_NAME} 
	PUBLIC 
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
)

add_subdirectory( tests )