###############################################################################
# IDE Utils
###############################################################################

# Copyright (c) 2017 - 2019 -- Élie Michel <elie.michel@exppad.com>
# Reproduce the original folder layout in IDE
function(group_source_by_folder)
    foreach(file ${ARGV}) 
        # Get the directory of the source file
        get_filename_component(parent_dir "${file}" DIRECTORY)

        # Remove common directory prefix to make the group
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" group "${parent_dir}")

        # Make sure we are using windows slashes
        string(REPLACE "/" "\\" group "${group}")

        # Group into "Source Files" and "Header Files"
        if ("${file}" MATCHES ".*\\.cpp")
           set(group "Source Files\\${group}")
        elseif("${file}" MATCHES ".*\\.h")
           set(group "Header Files\\${group}")
        endif()

        source_group("${group}" FILES "${file}")
    endforeach()
endfunction()

###############################################################################
# CMake Utils
###############################################################################

# Automatically generate moc files for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED CONFIG)

if(Qt5Widgets_FOUND)
	message("Using Qt ${Qt5Widgets_VERSION} from ${Qt5Core_DIR}")
endif()

find_package(Eigen3 CONFIG REQUIRED)

set(LIBS
	
	ann
	glad
	glm
	stb_image
	smooth_contours
	mikktspace
	
	Eigen3::Eigen

	Qt5::Core
	Qt5::Gui
	Qt5::Widgets
	)

set(SRCS

	main.cpp
					
	Window/ControlPanel.h
	Window/ControlPanel.cpp
	Window/MainWindow.h
	Window/MainWindow.cpp
	Window/ViewerWidget.h
	Window/ViewerWidget.cpp

	Utils/AlignedBox.h
	Utils/AlignedBox.cpp
	Utils/Camera.h
	Utils/Camera.cpp
	Utils/TangentSpace.h
	Utils/TangentSpace.cpp
			
	Utils/Contours.h
	Utils/Contours.cpp
	
	Warpgrid/KDTree.h
	Warpgrid/LinearSystem.h
	Warpgrid/Mat2.h
	Warpgrid/Solver.h
	Warpgrid/Solver.cpp
	
	Warpgrid/Warpgrid.h
	Warpgrid/Warpgrid.cpp
	Warpgrid/WarpIO.h
	Warpgrid/WarpIO.cpp
	Warpgrid/WarpUtils.h
	Warpgrid/WarpUtils.cpp
	
	Utils/Color.h
	Utils/Color.cpp
	Utils/Histogram.h
	Utils/Histogram.cpp
	Utils/MathUtils.h
	Utils/MathUtils.cpp
	Utils/NormalReorientation.h
	Utils/NormalReorientation.cpp

	Rendering/Debug.h
	Rendering/Debug.cpp
	Rendering/Mesh.h
	Rendering/Mesh.cpp
	Rendering/SceneState.h
	Rendering/Shader.h
	Rendering/Shader.cpp
	Rendering/Viewer.h
	Rendering/Viewer.cpp
	)

add_executable (MatMorpher "${SRCS}")

# For IDE projects
group_source_by_folder(${SRCS})

# Add include directories to a target in IDE
target_include_directories(MatMorpher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Specify libraries to use when linking a given target and/or its dependents
target_link_libraries(MatMorpher LINK_PRIVATE "${LIBS}" "${CMAKE_DL_LIBS}")

# select working dir for the msvc debugger
set_target_properties(MatMorpher PROPERTIES
	VS_DEBUGGER_WORKING_DIRECTORY  "${CMAKE_CURRENT_LIST_DIR}/../bin"
)

# select output dir for the exe
set_target_properties(MatMorpher PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_CURRENT_LIST_DIR}/../bin"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_CURRENT_LIST_DIR}/../bin"
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_LIST_DIR}/../bin"
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_CURRENT_LIST_DIR}/../bin"
)

# set_target_properties(MatMorpher PROPERTIES DEBUG_POSTFIX "-debug")
# set_target_properties(MatMorpher PROPERTIES RELEASE_POSTFIX "-release")
