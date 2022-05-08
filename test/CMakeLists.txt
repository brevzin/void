file(GLOB sources CONFIGURE_DEPENDS *.cxx)

add_executable(void-test ${sources})
target_link_libraries(void-test PUBLIC void)
add_test(NAME void-test COMMAND void-test)
