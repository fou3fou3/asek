cmake_minimum_required(VERSION 3.20)

project(asek VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Flatbuffers REQUIRED)

set(SCHEMA_FILE "${CMAKE_CURRENT_SOURCE_DIR}/schemas/tfIdfIndex.fbs")
set(GENERATED_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(GENERATED_HEADER "${GENERATED_OUTPUT_DIR}/tfidf_index_generated.h")

add_custom_command(
    OUTPUT ${GENERATED_HEADER}
    COMMAND flatbuffers::flatc --cpp -o ${GENERATED_OUTPUT_DIR} ${SCHEMA_FILE}
    DEPENDS ${SCHEMA_FILE}
    COMMENT "Generating C++ headers from FlatBuffers schema..."
)

add_executable(asek
    src/main.cpp
	src/index.cpp
	src/search.cpp

	${GENERATED_HEADER}
)

target_link_libraries(asek PRIVATE 
    imgui::imgui
    glfw
	OpenGL::GL
)

target_include_directories(asek PRIVATE include ${flatbuffers_INCLUDE_DIRS}
    ${GENERATED_OUTPUT_DIR})