cmake_minimum_required (VERSION 2.8)

#### USER DEFINED ##############################################################

#### BASIC SETTINGS ############################################################

include_directories(.)
# Compile with cmake -DCMAKE_BUILD_TYPE=RELEASE ..
set (CMAKE_CXX_FLAGS "-std=c++2a -msse4.2 -Wall -Wextra -O3")

# Compile with cmake -DCMAKE_BUILD_TYPE=DEBUG ..
set (CMAKE_CXX_FLAGS_DEBUG "-std=c++2a -msse4.2 -Wall -Wextra -g -fno-omit-frame-pointer -fsanitize=address")

set (CMAKE_CXX_STANDARD 20)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Boost 1.78.0 REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

#### TARGETS ###################################################################

add_executable(time_trie source/time_trie.cpp)
target_link_libraries(time_trie PRIVATE Threads::Threads)
target_link_libraries(time_trie PRIVATE OpenMP::OpenMP_CXX)

add_executable(time_levenshtein  source/time_levenshtein.cpp)
target_link_libraries(time_levenshtein PRIVATE Threads::Threads)
target_link_libraries(time_levenshtein PRIVATE OpenMP::OpenMP_CXX)

add_executable(time_levenshtein_query  source/time_levenshtein_query.cpp)
target_link_libraries(time_levenshtein_query PRIVATE Threads::Threads)
target_link_libraries(time_levenshtein_query PRIVATE OpenMP::OpenMP_CXX)

add_executable(test_trie source/test_trie.cpp)
target_link_libraries(test_trie PRIVATE Threads::Threads)
target_link_libraries(test_trie PRIVATE OpenMP::OpenMP_CXX)

add_executable(test_levenshtein  source/test_levenshtein.cpp)
target_link_libraries(test_levenshtein PRIVATE Threads::Threads)
target_link_libraries(test_levenshtein PRIVATE OpenMP::OpenMP_CXX)

add_executable(demo  source/demo.cpp)
target_link_libraries(demo PRIVATE Threads::Threads)
target_link_libraries(demo PRIVATE OpenMP::OpenMP_CXX)
