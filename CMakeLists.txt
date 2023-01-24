add_library(MicroNetworkHost INTERFACE)
target_include_directories(MicroNetworkHost INTERFACE Src INTERFACE ${LFRAMEWORK_CONFIG_FILE_DIR})
add_subdirectory(Src/MicroNetwork/Host)

target_include_directories(MicroNetworkHost INTERFACE ${API_DIR})

