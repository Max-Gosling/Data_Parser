cmake_minimum_required(VERSION 3.14)
project(Data_Parser)


set(UTILS_DIR "${CMAKE_SOURCE_DIR}/utilities")
set(COCO_EXE "${UTILS_DIR}/coco-src/Coco_exe.exe")
set(FRAMES_DIR "${UTILS_DIR}/coco-frames")   

set(GRAMMAR_FILE "${CMAKE_SOURCE_DIR}/src/parser.atg")

set(GENERATED_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/scanner.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/scanner.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/parser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/parser.h"
)


add_custom_command(
    OUTPUT ${GENERATED_FILES}
    COMMAND ${COCO_EXE} -frames ${FRAMES_DIR} ${GRAMMAR_FILE}
    DEPENDS ${GRAMMAR_FILE}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src"
    COMMENT "No comments, absolute cinema"
)


add_custom_target(generate_parser ALL DEPENDS ${GENERATED_FILES})


add_executable(Data_Parser
    src/main.cpp
    ${GENERATED_FILES}      
)


add_dependencies(Data_Parser generate_parser)