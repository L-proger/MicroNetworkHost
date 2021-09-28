add_library(MicroNetworkHost STATIC)
set_property(TARGET MicroNetworkHost PROPERTY CXX_STANDARD 17)
target_include_directories(MicroNetworkHost PUBLIC Src PRIVATE ${LFRAMEWORK_CONFIG_FILE_DIR})
add_subdirectory(Src/MicroNetwork/Host)

target_include_directories(MicroNetworkHost PRIVATE ${API_DIR})

