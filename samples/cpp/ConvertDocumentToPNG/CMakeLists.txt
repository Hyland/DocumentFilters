cmake_minimum_required(VERSION 3.15)

add_executable (ConvertDocumentToPNG "ConvertDocumentToPNG.cpp")
target_link_libraries (ConvertDocumentToPNG PUBLIC DocumentFiltersSamples)