cmake_minimum_required(VERSION 3.15)

add_executable (ConvertDocumentToXML "ConvertDocumentToXML.cpp")
target_link_libraries (ConvertDocumentToXML PUBLIC DocumentFiltersSamples)
install(TARGETS ConvertDocumentToXML DESTINATION bin)