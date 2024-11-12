cmake_minimum_required(VERSION 3.15)

add_executable (GetDocumentType "GetDocumentType.cpp")
target_link_libraries (GetDocumentType PUBLIC DocumentFiltersSamples)
install(TARGETS GetDocumentType DESTINATION bin)