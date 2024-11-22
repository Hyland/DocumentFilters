cmake_minimum_required(VERSION 3.15)

add_executable (ConvertDocumentToUTF8WithOCR "ConvertDocumentToUTF8WithOCR.cpp")
target_link_libraries (ConvertDocumentToUTF8WithOCR PUBLIC DocumentFiltersSamples)
install(TARGETS ConvertDocumentToUTF8 DESTINATION bin)
