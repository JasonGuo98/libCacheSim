
add_executable(mrcProfiler main.cpp mrcProfiler.cpp cli_parser.cpp ../cli_reader_utils.c)
target_link_libraries(mrcProfiler ${ALL_MODULES} ${LIBS} ${CMAKE_THREAD_LIBS_INIT} utils)
install(TARGETS mrcProfiler RUNTIME DESTINATION bin)

set_target_properties(mrcProfiler
        PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)
