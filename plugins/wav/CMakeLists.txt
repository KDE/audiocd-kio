add_library(audiocd_encoder_wav SHARED)

target_sources(audiocd_encoder_wav PRIVATE
    encoderwav.cpp
    encodercda.cpp
)

target_include_directories(audiocd_encoder_wav
    PRIVATE
        ${CMAKE_BINARY_DIR}
)

target_link_libraries(audiocd_encoder_wav 
    audiocdplugins 
)

install(TARGETS audiocd_encoder_wav  DESTINATION ${KDE_INSTALL_PLUGINDIR})
