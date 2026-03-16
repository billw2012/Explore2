
set(src_files
	explore2remote.cpp
	main.cpp
	)

set(header_files
	explore2remote.h
	)

qt5_wrap_ui(ui_files explore2remote.ui)

qt5_add_resources(resource_files explore2remote.qrc)

include_directories(..)

add_executable(Explore2Remote ${src_files} ${header_files} ${ui_files} ${resource_files} remote.rc)

target_link_libraries(Explore2Remote Remote Ugly debug qwtd.lib optimized qwt.lib)

qt5_use_modules(Explore2Remote Core Gui Widgets)

set(INSTALL_DIR ${CMAKE_BINARY_DIR}/../bin)

#macro(_installDLL QT_MODULE)
#	get_target_property(DLL_LOCATION Qt5::${QT_MODULE} IMPORTED_LOCATION_DEBUG)
#	install(FILES ${DLL_LOCATION} DESTINATION ${INSTALL_DIR} CONFIGURATIONS DEBUG)
#	get_target_property(DLL_LOCATION Qt5::${QT_MODULE} IMPORTED_LOCATION_RELEASE)
#	install(FILES ${DLL_LOCATION} DESTINATION ${INSTALL_DIR} CONFIGURATIONS RELEASE RELWITHDEBINFO)
#endmacro()

#_installDLL(Core)
#_installDLL(Gui)
#_installDLL(Widgets)

#set(QTDIR $ENV{QTDIR})
#STRING(REGEX REPLACE "\\\\" "/" QTDIR ${QTDIR}) 

#install(FILES "${QTDIR}/bin/icuin51.dll" DESTINATION ${INSTALL_DIR})
#install(FILES "${QTDIR}/bin/icuuc51.dll" DESTINATION ${INSTALL_DIR})
#install(FILES "${QTDIR}/bin/icudt51.dll" DESTINATION ${INSTALL_DIR})

install(TARGETS Explore2Remote RUNTIME DESTINATION ${INSTALL_DIR})
