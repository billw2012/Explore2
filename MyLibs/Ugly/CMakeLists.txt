
set(src_files
	ProcessQueue.cpp
	ProcessRedirect.cpp
	QDebugStream.cpp
	ugly.cpp
	UglyGraphicsView.cpp
	)

set(header_files
	ProcessQueue.h
	ProcessRedirect.h
	QDebugStream.h
	QtUtils.h
	temp.h
	ugly.h
	UglyGraphicsView.h
	wait_handle.h	
	)
	
qt5_wrap_ui(ui_files FCapGraphicsView.ui)

include_directories(..)

add_library(Ugly ${src_files} ${header_files} ${ui_files})

qt5_use_modules(Ugly Core Gui Widgets)
