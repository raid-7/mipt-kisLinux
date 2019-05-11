cmake_minimum_required(VERSION 3.5)

set(CMAKE_C_STANDARD 11)

find_package(Threads REQUIRED)
link_libraries(${CMAKE_THREAD_LIBS_INIT})

add_library(rfscore STATIC src/fs.c src/manage.c src/myutil.c)
target_include_directories(rfscore PUBLIC include)

include(preprocessor.cmake)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated/include)
preprocess_file(
        INPUT  include/rfsnet/operation_defines.h.in
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/include/operation_defines.h
)

add_library(rfsnet STATIC
        src/net_backend.c src/backend_operations_impl.c src/serialization.c src/serializers.c src/net_frontend.c)
target_include_directories(rfscore PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/generated/include)
target_link_libraries(rfsnet rfscore)

add_executable(rfscli src/main_cli.c)
target_link_libraries(rfscli rfsnet)

add_executable(rfsdaemon src/main_backend.c)
target_link_libraries(rfsdaemon rfsnet)
