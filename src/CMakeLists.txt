INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

# ########## bordeaux-mqtt-bridge executable ##########
# Sources:
SET(bordeaux_mqtt-bridge_SRCS
    main.c
    configuration.c
    logging.c
    iniparser/iniparser.c
    iniparser/dictionary.c
)

# Headers:
SET(bordeaux_mqtt-bridge_HDRS
    configuration.h
    logging.h
    iniparser/iniparser.h
    iniparser/dictionary.h
)


## add the relative path as __FILENAME__ macro
## as with cmake __FILE__ will be the absolute path
foreach(f IN LISTS bordeaux_mqtt-bridge_SRCS)
 get_filename_component(b ${f} NAME)
  set_source_files_properties(${f} PROPERTIES
  COMPILE_DEFINITIONS "__FILENAME__=\"${b}\"")
  ## ^^ might be gcc dependant ^^
endforeach()

## libcurl stuff
#find_package(libcurl REQUIRED)
FIND_PACKAGE(CURL)
include_directories(${CURL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${CURL_LIBRARIES})

set(LIBS ${LIBS} mosquitto)

## actual target:
ADD_EXECUTABLE(bordeaux-mqtt-bridge ${bordeaux_mqtt-bridge_SRCS})
target_link_libraries(bordeaux-mqtt-bridge ${LIBS})

# add install target:
INSTALL(TARGETS bordeaux-mqtt-bridge DESTINATION usr/bin)
