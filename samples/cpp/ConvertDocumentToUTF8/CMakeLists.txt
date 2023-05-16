cmake_minimum_required(VERSION 3.15)

add_executable (ConvertDocumentToUTF8 "ConvertDocumentToUTF8.cpp")
target_link_libraries (ConvertDocumentToUTF8 PUBLIC DocumentFiltersSamples)