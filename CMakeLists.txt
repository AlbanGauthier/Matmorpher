cmake_minimum_required(VERSION 3.1...3.17)

# set the project name
project(MatMorpher VERSION 1.0
            DESCRIPTION "Code for the paper MatMorpher: A Morphing Operator for SVBRDFs"
            LANGUAGES CXX C)
			
# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# We first include external libraries on which this project depends
add_subdirectory(libs)

# Then we build our project
add_subdirectory(src)

# Allow for using folders in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# launch vs studio with the right project selected
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MatMorpher)
