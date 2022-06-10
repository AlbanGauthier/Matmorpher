set(SOURCES

	#smooth_contours_cmd.c
	smooth_contours.c 
	smooth_contours.h
	io.c 
	io.h 
	)

add_library(smooth_contours ${SOURCES})

target_include_directories(smooth_contours PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# STANDALONE CMD BUILD

add_executable(contours_cmd smooth_contours_cmd.c)

set(LIBS
	smooth_contours
)

if(CMAKE_COMPILER_IS_GNUCC)
	list(APPEND LIBS m)
endif(CMAKE_COMPILER_IS_GNUCC)

target_link_libraries(contours_cmd PRIVATE ${LIBS})