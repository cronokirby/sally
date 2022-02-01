# We need version 3.12 to not have to rerun cmake when our glob changes
cmake_minimum_required(VERSION 3.12)

project(Sally)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# CONFIGURE_DEPENDS makes it so that whenever this changes, we can just
# run `make`, without having to rerun `cmake` all over again.
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "src/*.c")

add_executable(sally ${sources})

if (CMAKE_BUILD_TYPE MATCHES RELEASE)
  set_property(TARGET sally PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  target_compile_options(sally PRIVATE -Werror -Wall -Wextra -O3 -DNDEBUG) 
else()
  target_compile_options(sally PRIVATE -Werror -Wall -Wextra -fsanitize=address -g)
  target_link_libraries(sally -fsanitize=address)
endif (CMAKE_BUILD_TYPE MATCHES RELEASE)

