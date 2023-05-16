cmake_minimum_required(VERSION 3.15)

add_library (DocumentFiltersObjects STATIC DocumentFiltersObjects.cpp DocumentFiltersObjects.h) 

target_compile_features(DocumentFiltersObjects INTERFACE cxx_std_11)
target_include_directories(DocumentFiltersObjects PUBLIC ..)
target_include_directories(DocumentFiltersObjects INTERFACE . ..)