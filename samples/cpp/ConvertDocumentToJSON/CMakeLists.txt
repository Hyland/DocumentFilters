cmake_minimum_required(VERSION 3.15)

add_executable (ConvertDocumentToJSON "ConvertDocumentToJSON.cpp")
target_link_libraries (ConvertDocumentToJSON PUBLIC DocumentFiltersSamples)
