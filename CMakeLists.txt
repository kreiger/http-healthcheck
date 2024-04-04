cmake_minimum_required(VERSION 3.10)

project(http-healthcheck)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")

add_executable(http-healthcheck http-healthcheck.c)

add_custom_command(TARGET http-healthcheck POST_BUILD
        COMMAND ${CMAKE_STRIP} http-healthcheck
        COMMENT "Stripping binary")

add_custom_target(docker
        COMMAND docker build -t http-healthcheck .
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Building Docker image..."
)
