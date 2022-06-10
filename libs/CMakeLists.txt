# Glad (v0.1.27), providing headers for OpenGL procedures
set(GLAD_PROFILE "core" CACHE STRING "" FORCE)
set(GLAD_API "gl=4.5,gles2=" CACHE STRING "" FORCE)
add_subdirectory(glad)

# glm : OpenGL Mathematics
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE glm)

# stb_image
add_subdirectory(stb_image)

# smooth contours
add_subdirectory(smooth_contours_1.0)

# ANN: Approximate Nearest Neighbors
add_subdirectory(ann_1.1.2)

# mikktspace
add_subdirectory(mikktspace)

set_property(TARGET glad 				PROPERTY FOLDER "Libs")
set_property(TARGET glad-generate-files PROPERTY FOLDER "Libs")
set_property(TARGET stb_image 			PROPERTY FOLDER "Libs")
set_property(TARGET smooth_contours		PROPERTY FOLDER "Libs")
set_property(TARGET ann					PROPERTY FOLDER "Libs")
set_property(TARGET mikktspace 			PROPERTY FOLDER "Libs")