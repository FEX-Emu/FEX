target_sources(CommonWindowsRuntime PRIVATE Alloc.cpp IO.cpp Math.cpp String.cpp Misc.cpp CRT.cpp)
add_subdirectory(Musl)
