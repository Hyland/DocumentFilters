include(ExternalProject)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

# Set the URL of the ZIP file you want to download
set(EXTERNAL_PROJECT_NAME DocumentFiltersBinaries)
set(EXTERNAL_PROJECT_VERSION v23.3)
set(EXTERNAL_PROJECT_PREFIX ${CMAKE_BINARY_DIR}/${EXTERNAL_PROJECT_NAME})

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(EXTERNAL_PROJECT_URL https://github.com/HylandSoftware/DocumentFilters/releases/download/${EXTERNAL_PROJECT_VERSION}/windows-intel-msvc-64.zip)
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(EXTERNAL_PROJECT_URL https://github.com/HylandSoftware/DocumentFilters/releases/download/${EXTERNAL_PROJECT_VERSION}/linux-intel-gcc-64.zip)
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(EXTERNAL_PROJECT_URL https://github.com/HylandSoftware/DocumentFilters/releases/download/${EXTERNAL_PROJECT_VERSION}/macos-intel-clang-64.zip)
else()
    message(FATAL_ERROR "Unsupported operating system: ${CMAKE_SYSTEM_NAME}")
endif()

# Configure and build the external project
ExternalProject_Add(
    ${EXTERNAL_PROJECT_NAME}
    PREFIX ${EXTERNAL_PROJECT_PREFIX}
    URL ${EXTERNAL_PROJECT_URL}
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

# Define a custom target to execute after building the external project
add_custom_target(${EXTERNAL_PROJECT_NAME}_unzip
    COMMAND ${CMAKE_COMMAND} -E tar xf ${EXTERNAL_PROJECT_PREFIX}/src/${EXTERNAL_PROJECT_NAME}.zip
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS ${EXTERNAL_PROJECT_NAME}
)

# Define a custom target to copy the contents to the bin directory
add_custom_target(${EXTERNAL_PROJECT_NAME}_copy
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXTERNAL_PROJECT_PREFIX}/src/${EXTERNAL_PROJECT_NAME} ${CMAKE_BINARY_DIR}/bin
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS ${EXTERNAL_PROJECT_NAME}
    # DEPENDS ${EXTERNAL_PROJECT_NAME}_unzip
)


