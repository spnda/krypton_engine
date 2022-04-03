# This will search for all files with .c, .cpp, .h, .hpp extensions
# in the given folder directory but no subdirectories and add them
# as sources to the target.
function(add_source_directory)
    cmake_parse_arguments(PARAM "" "TARGET;FOLDER" "CONDITIONAL" ${ARGN})

    file(GLOB TARGET_SOURCES ${PARAM_FOLDER}/*.c ${PARAM_FOLDER}/*.cpp ${PARAM_FOLDER}/*.mm)
    file(GLOB TARGET_HEADERS ${PARAM_FOLDER}/*.h ${PARAM_FOLDER}/*.hpp)

    foreach (SOURCE ${TARGET_SOURCES})
        target_sources(${PARAM_TARGET} PRIVATE ${SOURCE})
    endforeach()
    foreach (HEADER ${TARGET_HEADERS})
        target_sources(${PARAM_TARGET} PRIVATE ${HEADER})
    endforeach()
endfunction()
