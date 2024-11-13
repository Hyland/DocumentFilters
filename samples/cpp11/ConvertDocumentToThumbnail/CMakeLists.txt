cmake_minimum_required(VERSION 3.15)

add_executable (ConvertDocumentToThumbnail "ConvertDocumentToThumbnail.cpp")
target_link_libraries (ConvertDocumentToThumbnail PUBLIC DocumentFiltersSamples)
install(TARGETS ConvertDocumentToThumbnail DESTINATION bin) 