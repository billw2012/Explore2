
set(src_files
	commontypes.cpp
	chunkdatathread.cpp
	databuilderthread.cpp
	heightdataprovider.cpp
	textureprovider.cpp
	chunklod.cpp
	planetgeometry.cpp
	solarsystem.cpp
	atmospherics.cpp
	errorlog.cpp
	texturearraymanager.cpp
	gbuffer.cpp
	planet.cpp
	orbit.cpp
	sungeometry.cpp
	geometries.cpp
	atmospheric_scattering.cpp
	player.cpp
	)
	
set(header_files
	commontypes.h
	chunkbatcher.h
	chunkdatathread.h
	databuilderthread.h
	heightdataprovider.h
	textureprovider.h
	chunklod.h
	planetgeometry.h
	solarsystem.h
	atmospherics.h
	texturearraymanager.h
	gbuffer.h
	planet.h
	orbit.h
	sungeometry.h
	geometries.h
	atmospheric_scattering.h
	player.h
	engine_constants.h
	)

include_directories(..)


add_library(Explore2Engine ${src_files} ${header_files})
