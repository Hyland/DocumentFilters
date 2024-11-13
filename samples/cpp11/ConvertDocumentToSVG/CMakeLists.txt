cmake_minimum_required(VERSION 3.15)

add_executable (ConvertDocumentToSVG "ConvertDocumentToSVG.cpp")
target_link_libraries (ConvertDocumentToSVG PUBLIC DocumentFiltersSamples)
install(TARGETS ConvertDocumentToSVG DESTINATION bin)