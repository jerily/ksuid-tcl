cmake_minimum_required(VERSION 3.22.1)
project(ksuid-tcl VERSION 1.0.5 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(WIN32)
    set(CMAKE_SHARED_LIBRARY_PREFIX "lib")
    set(CMAKE_STATIC_LIBRARY_PREFIX "lib")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
find_package(TclStub 8.6 REQUIRED)

add_library(${PROJECT_NAME} src/library.cc src/base62.cc src/hex.cc src/custom_unt128.cc)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(${PROJECT_NAME} PRIVATE ${TCL_INCLUDE_PATH})
target_compile_definitions(${PROJECT_NAME} PRIVATE TCL_THREADS PROJECT_VERSION=${PROJECT_VERSION})

if(BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_TCL_STUBS)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${TCL_STUB_LIBRARY})
else()
    # The embedding application links Tcl and registers the init function.
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATIC_BUILD)
endif()

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib/${PROJECT_NAME}${PROJECT_VERSION}
    ARCHIVE DESTINATION lib/${PROJECT_NAME}${PROJECT_VERSION}
    RUNTIME DESTINATION lib/${PROJECT_NAME}${PROJECT_VERSION})

if(BUILD_SHARED_LIBS)
    configure_file(pkgIndex.tcl.in pkgIndex.tcl @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pkgIndex.tcl
        DESTINATION lib/${PROJECT_NAME}${PROJECT_VERSION})
endif()

include(CTest)
if(BUILD_TESTING AND BUILD_SHARED_LIBS)
    if(NOT TCL_TCLSH)
        message(FATAL_ERROR "Tcl shell required for tests; set TCL_TCLSH or BUILD_TESTING=OFF")
    endif()
    add_test(NAME AllUnitTests COMMAND ${TCL_TCLSH}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/all.tcl ${CMAKE_CURRENT_BINARY_DIR})
endif()
