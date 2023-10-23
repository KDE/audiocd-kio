cmake_minimum_required(VERSION 3.16)

project(audiocd VERSION "5.1.0")

set(QT_MIN_VERSION "6.5.0")
set(KF_MIN_VERSION "5.240.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEClangFormat)

include(ECMSetupVersion)
include(ECMQtDeclareLoggingCategory)
include(CMakePushCheckState)
include(GenerateExportHeader)
include(CheckStructHasMember)
include(FeatureSummary)
include(KDEGitCommitHooks)
include(ECMDeprecationSettings)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Widgets)
find_package(Qt6 REQUIRED COMPONENTS Core5Compat)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Config
    I18n
    DocTools
    KIO
    KCMUtils
)
find_package(KCompactDisc6)
set_package_properties(KCompactDisc6 PROPERTIES
    DESCRIPTION "KCompactDisc library"
    URL "https://commits.kde.org/libkcompactdisc"
    TYPE REQUIRED
    PURPOSE "libkcompactdisc is used to access CD drives."
)
find_package(KCddb6 5.1)
set_package_properties(KCddb6 PROPERTIES
    DESCRIPTION "Library used to retrieve audio CD metadata from the Internet"
    URL "https://commits.kde.org/libkcddb"
    TYPE REQUIRED
    PURPOSE "libkcddb is used to retrieve audio CD meta data from the internet."
)

find_package(Cdparanoia REQUIRED)

cmake_push_check_state()
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${CDPARANOIA_INCLUDE_DIR})
check_struct_has_member("struct cdrom_drive" "ioctl_device_name" "cdda_interface.h" CDDA_IOCTL_DEVICE_EXISTS)
cmake_pop_check_state()
if(CDDA_IOCTL_DEVICE_EXISTS)
    set(HAVE_CDDA_IOCTL_DEVICE 1)
else()
    set(HAVE_CDDA_IOCTL_DEVICE 0)
endif()
execute_process(
    COMMAND sed -e "s|cdda_private_data_t \\*private;|cdda_private_data_t \\*private_data;|g"
    # The next line should be removed if strerror_tr will be used somewhere
    COMMAND sed -e "/static\ char/,/}\;/d"
    INPUT_FILE ${CDPARANOIA_INCLUDE_DIR}/cdda_interface.h
    OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/cdda_interface.hpp
    RESULT_VARIABLE CDDA_INTERFACE_EDIT_ERROR
)
if (CDDA_INTERFACE_EDIT_ERROR)
    message(STATUS "Fixing cdda_interface.h for C++ failed with exit code ${CDDA_INTERFACE_EDIT_ERROR}")
endif()

configure_file(config-audiocd.h.in ${CMAKE_CURRENT_BINARY_DIR}/config-audiocd.h)

#############################
ecm_set_disabled_deprecation_versions(QT 6.6
    KF 5.240.0
    )
add_subdirectory(plugins)
add_subdirectory(kcmaudiocd)
add_subdirectory(data)
add_subdirectory(doc)

add_library(kio_audiocd MODULE)
target_sources(kio_audiocd PRIVATE
    audiocd.cpp audiocd.h
)

ecm_qt_declare_logging_category(kio_audiocd
    HEADER audiocd_kio_debug.h
    IDENTIFIER AUDIOCD_KIO_LOG
    CATEGORY_NAME kf.kio.workers.audiocd
    OLD_CATEGORY_NAMES kf5.kio.audiocd kf.kio.slaves.audiocd
    DEFAULT_SEVERITY Warning
    DESCRIPTION "audiocd KIO worker"
    EXPORT AUDIOCD
)

set_target_properties(kio_audiocd PROPERTIES OUTPUT_NAME "audiocd")

target_link_libraries(kio_audiocd
    audiocdplugins
    KF6::I18n
    Qt::Widgets
    Cdparanoia::Cdparanoia
)
target_link_libraries(kio_audiocd Qt6::Core5Compat KCompactDisc6)

install(TARGETS kio_audiocd  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/kio)

ecm_qt_install_logging_categories(
    EXPORT AUDIOCD
    FILE kio_audiocd.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)

ki18n_install(po)
kdoctools_install(po)
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
