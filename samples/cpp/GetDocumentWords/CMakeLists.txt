cmake_minimum_required(VERSION 3.15)

add_executable (GetDocumentWords "GetDocumentWords.cpp")
target_link_libraries (GetDocumentWords PUBLIC DocumentFiltersSamples)