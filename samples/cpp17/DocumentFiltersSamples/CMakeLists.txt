cmake_minimum_required(VERSION 3.15)

set(PROJECT_NAME "DocumentFiltersSamples")
add_library (${PROJECT_NAME} STATIC DocumentFiltersSamples.cpp DocumentFiltersSamples.h) 

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_include_directories(${PROJECT_NAME} PUBLIC ..)
target_include_directories(${PROJECT_NAME} INTERFACE . ..)
target_link_libraries (${PROJECT_NAME} PUBLIC DocumentFilters::Cpp17)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Samples")

