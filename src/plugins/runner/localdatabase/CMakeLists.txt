PROJECT( LocalDatabasePlugin)

INCLUDE_DIRECTORIES(
 ${CMAKE_CURRENT_SOURCE_DIR}
 ${CMAKE_CURRENT_BINARY_DIR}
)

set(localdatabase_SRCS LocalDatabaseRunner.cpp LocalDatabasePlugin.cpp LocalDatabaseRunner.h LocalDatabasePlugin.h)

marble_add_plugin( LocalDatabasePlugin ${localdatabase_SRCS})
