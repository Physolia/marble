PROJECT( LatLonEditPlugin)

INCLUDE_DIRECTORIES(
 ${CMAKE_CURRENT_SOURCE_DIR}
 ${CMAKE_CURRENT_BINARY_DIR}
 ${Qt6UiPlugin_INCLUDE_DIRS}
 ${Qt6Designer_INCLUDE_DIRS}
)

set(latloneditplugin_SRCS LatLonEditPlugin.cpp LatLonEditPlugin.h)

marble_add_designer_plugin( LatLonEditPlugin ${latloneditplugin_SRCS})
