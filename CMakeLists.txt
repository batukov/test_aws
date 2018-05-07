cmake_minimum_required(VERSION 3.9)
project(test_aws)


find_package(AWSSDK)

Message(STATUS "AWSSDK_LIB_DIR: ${AWSSDK_LIB_DIR}")
Message(STATUS "AWSSDK_INC_DIR: ${AWSSDK_INCLUDE_DIR}")
Message(STATUS "AWSSDK_BIN_DIR: ${AWSSDK_BIN_DIR}")

SET(SERVICE_LIST sqs s3 transfer dynamodb s3-encryption apigateway)
AWSSDK_CPY_DYN_LIBS(SERVICE_LIST "" ./)


SET(SERVICE s3-encryption transfer apigateway)

AWSSDK_LIB_DEPS(SERVICE DEPS)
Message(STATUS "${SERVICE} depends on libs: ${DEPS}")

AWSSDK_DETERMINE_LIBS_TO_LINK(SERVICE OUTPUT)

Message(STATUS "All linked libs: ${OUTPUT}")

if (NOT WIN32)
    find_package(PkgConfig)
    pkg_check_modules(DEPS REQUIRED ${OUTPUT})
endif()

if (APPLE)
    add_definitions(-DUNDER_MACOS)
endif()

link_directories("${AWSSDK_LIB_DIR}")
add_executable(test_aws main.cpp)

if (WIN32)
    add_definitions(-DUSE_IMPORT_EXPORT)
    target_link_libraries(test_aws ${OUTPUT})
    target_include_directories(test_aws PUBLIC ${AWSSDK_INCLUDE_DIR})
else()
    target_link_libraries(test_aws ${DEPS_LIBRARIES})
    target_include_directories(test_aws PRIVATE ${DEPS_INCLUDE_DIRS})
    target_compile_options(test_aws PRIVATE ${DEPS_CFLAGS_OTHER})
endif()
execute_process(COMMAND echo export PATH=$PATH:/${CMAKE_BINARY_DIR}/test_aws » ~/.bashrc WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
execute_process(COMMAND source ~/.bashrc WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
# add the install targets
install(TARGETS test_aws
        CONFIGURATIONS Debug
        RUNTIME DESTINATION /usr/local/bin)