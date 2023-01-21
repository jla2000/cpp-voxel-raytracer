set(
        SOURCE_FILES
        main.cpp
        rendering/AccelerationStructure.cpp
        rendering/Shader.cpp
        rendering/Camera.cpp
        rendering/VoxelBuffer.cpp
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(draft ${SOURCE_FILES})
target_link_libraries(draft ${DRAFT_LIBS})