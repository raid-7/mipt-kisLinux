project(directory_kmodule_test)
set(CMAKE_CXX_STANDARD 14)

add_executable(ch_dircli lltest.cpp)
add_executable(sys_dircli systest.cpp calls.c)
