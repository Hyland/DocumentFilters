cmake_minimum_required(VERSION 3.15)

add_executable (ConvertDocumentToTIFF "ConvertDocumentToTIFF.cpp")
target_link_libraries (ConvertDocumentToTIFF PUBLIC DocumentFiltersSamples)