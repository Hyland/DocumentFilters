cmake_minimum_required(VERSION 3.15)

add_library (DocumentFiltersSamples STATIC DocumentFiltersSamples.cpp DocumentFiltersSamples.h) 

target_compile_features(DocumentFiltersSamples INTERFACE cxx_std_11)
target_include_directories(DocumentFiltersSamples PUBLIC ..)
target_include_directories(DocumentFiltersSamples INTERFACE . ..)
target_link_libraries(DocumentFiltersSamples PUBLIC DocumentFiltersObjects) 

add_dependencies(DocumentFiltersSamples DocumentFiltersBinaries)