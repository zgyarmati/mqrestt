project(bordeaux-mqtt-bridge)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.9)


## we got some custom packages to find
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

subdirs(src)

install(FILES bordeaux-mqtt-bridge.ini DESTINATION etc/ COMPONENT config)


# read debian/changelog and parse the version/build number
#FILE (READ "${CMAKE_CURRENT_SOURCE_DIR}/debian/changelog" DEBCHANGELOG)


## VERSION MAGIC
#STRING (REGEX MATCH "([0-9]+\\.[0-9]+\\.[0-9]+)"   DEBVERSION "${DEBCHANGELOG}")
#MESSAGE( "DEBIAN VERSION is ${DEBVERSION}" )


execute_process(
	COMMAND git log -1 --format=%h
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	OUTPUT_VARIABLE GITVERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
MESSAGE( "GIT VERSION is ${GITVERSION}" )

add_definitions( -DPACKAGE_NAME="bordeaux-mqtt-bridge" )
add_definitions( -DPACKAGE_VERSION="${DEBVERSION}" )
add_definitions( -DGIT_VERSION="${GITVERSION}")


#SET(CPACK_GENERATOR "DEB")
#SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Zoltan Gyarmati") #required
 
# INCLUDE(CPack)



