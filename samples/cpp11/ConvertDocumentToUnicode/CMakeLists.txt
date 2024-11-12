cmake_minimum_required(VERSION 3.15)

add_executable (ConvertDocumentToUnicode "ConvertDocumentToUnicode.cpp")
target_link_libraries (ConvertDocumentToUnicode PUBLIC DocumentFiltersSamples)
install(TARGETS ConvertDocumentToUnicode DESTINATION bin)