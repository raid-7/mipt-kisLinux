cmake_minimum_required(VERSION 3.5)

set(CMAKE_C_STANDARD 11)

add_library(rfscore SHARED src/fs.c src/manage.c src/myutil.c)
target_include_directories(rfscore PUBLIC include)

add_executable(rfs src/main.c)
target_link_libraries(rfs rfscore)
