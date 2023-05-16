cmake_minimum_required(VERSION 3.15)

add_library (DocumentFiltersSamples STATIC DocumentFiltersSamples.cpp DocumentFiltersSamples.h) 

target_compile_features(DocumentFiltersSamples INTERFACE cxx_std_11)
target_include_directories(DocumentFiltersSamples PUBLIC ..)
target_include_directories(DocumentFiltersSamples INTERFACE . ..)
target_link_libraries (DocumentFiltersSamples PUBLIC DocumentFiltersObjects)

if (WIN32)
	target_link_directories(DocumentFiltersSamples PUBLIC ..)
	if(${CMAKE_CXX_COMPILER_ARCHITECTURE_ID} MATCHES "X86")		
		target_link_libraries(DocumentFiltersSamples PUBLIC "ISYS11df_x86")
	else()
		target_link_libraries(DocumentFiltersSamples PUBLIC "ISYS11df_x64")
	endif()
else()
	target_link_libraries(DocumentFiltersSamples PUBLIC "ISYS11df")
endif()