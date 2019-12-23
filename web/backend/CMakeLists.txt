cmake_minimum_required(VERSION 3.5)
project(websocket-example)
 
include_directories(/usr/include)
 
set(CMAKE_CXX_FLAGS "-w -pthread")
 
#set(SF_CLIENT client.c)
set(SF_SERVER server.cpp)
 
#add_executable(client ${SF_CLIENT})
#target_link_libraries(client /home/alex/CPP/lib/libwebsockets/lib/libwebsockets.so)
  
add_executable(server ${SF_SERVER})

target_include_directories(server
	PUBLIC
	. ${J1939_SOURCE_DIR}/include ${Can_SOURCE_DIR}/include ${Common_SOURCE_DIR}/include ${LIBWEBSOCKETS_INCLUDE_DIR}
)

target_link_libraries(server
	PUBLIC
	${LIBWEBSOCKETS_LIBRARIES}
	jsoncpp
	J1939
	Can
)
