cmake_minimum_required(VERSION 3.15)

add_executable (ConvertDocumentToPDF "ConvertDocumentToPDF.cpp")
target_link_libraries (ConvertDocumentToPDF PUBLIC DocumentFiltersSamples)