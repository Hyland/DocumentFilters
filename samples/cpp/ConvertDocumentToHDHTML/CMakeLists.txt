cmake_minimum_required(VERSION 3.15)

add_executable (ConvertDocumentToHDHTML "ConvertDocumentToHDHTML.cpp")
target_link_libraries (ConvertDocumentToHDHTML PUBLIC DocumentFiltersSamples)