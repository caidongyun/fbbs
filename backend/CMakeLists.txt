set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
add_definitions(${FB_XOPEN_SOURCE_DEFINE} ${FB_PLATFORM_DEFINE})

add_executable(bbsbed main.c post.c)
add_dependencies(bbsbed s11n)
target_link_libraries(bbsbed fbbs)
install(TARGETS bbsbed RUNTIME DESTINATION bin)
