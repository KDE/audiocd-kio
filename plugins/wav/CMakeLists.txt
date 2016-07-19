include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..  ${CDPARANOIA_INCLUDE_DIR})

########### next target ###############

set(audiocd_encoder_wav_PART_SRCS encoderwav.cpp encodercda.cpp)

add_library(audiocd_encoder_wav ${audiocd_encoder_wav_PART_SRCS})
set_target_properties(audiocd_encoder_wav PROPERTIES PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}")

target_link_libraries(audiocd_encoder_wav 
    audiocdplugins 
)

install(TARGETS audiocd_encoder_wav  DESTINATION ${PLUGIN_INSTALL_DIR})

