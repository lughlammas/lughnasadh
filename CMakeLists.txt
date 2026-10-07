cmake_minimum_required(VERSION 3.16)
project(lughnasadh VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(lughnasadh
  src/main.cpp
  src/bitboard.cpp
  src/position.cpp
  src/movegen.cpp
  src/evaluate.cpp
  src/tt.cpp
  src/search.cpp
  src/uci.cpp
)

target_include_directories(lughnasadh PRIVATE src)

if(MSVC)
  target_compile_options(lughnasadh PRIVATE /O2 /EHsc)
else()
  target_compile_options(lughnasadh PRIVATE -O3 -Wall -Wextra -Wno-unused-parameter)
  # Bionic already provides pthread; do not link libpthread on Android.
  if(NOT ANDROID)
    target_link_libraries(lughnasadh PRIVATE pthread)
  endif()
endif()

install(TARGETS lughnasadh RUNTIME DESTINATION bin)
