cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 11)

project(PBR)

find_package(OpenMP REQUIRED)

set(CMAKE_BUILD_TYPE  "Release")

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}
)

# include files
INCLUDE_DIRECTORIES(
	${QT_INCLUDE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
)

# Main GUI files
set(Main
	Main/main.cpp
)
# Make the MainGUI group
SOURCE_GROUP("Main" FILES ${Main})

# Core files
set(Core
	Core/PBR.h
	# 数据存储
	Core/FrameBuffer.h
	Core/FrameBuffer.cpp
	# 基本数据类型
	Core/Geometry.h
	Core/Geometry.cpp
	Core/Interaction.h
	Core/Interaction.cpp
	Core/Primitive.h
	Core/Primitive.cpp
	Core/Transform.h
	Core/Transform.cpp    
	Core/Spectrum.h
	Core/Spectrum.cpp
	Core/Scene.h
	Core/Scene.cpp
)
# Make the Core group
SOURCE_GROUP("Core" FILES ${Core})

# Accelerator files
set(Accelerator
	Accelerator/BVHAccel.h
	Accelerator/BVHAccel.cpp
)
# Make the Accelerator group
SOURCE_GROUP("Accelerator" FILES ${Accelerator})


set(Shape
	Shape/Shape.h
	Shape/Shape.cpp
	Shape/Sphere.h
	Shape/Sphere.cpp
	Shape/Triangle.h
	Shape/Triangle.cpp
)
# Make the Shape group
SOURCE_GROUP("Shape" FILES ${Shape})

# Camera files
set(Camera
	Camera/Camera.h
	Camera/Camera.cpp
	Camera/orthographic.h
	Camera/orthographic.cpp
 	Camera/Perspective.h
	Camera/perspective.cpp
)
# Make the Camera group
SOURCE_GROUP("Camera" FILES ${Camera})

set(Sampler
	Sampler/TimeClockRandom.h
	Sampler/Sampling.h
	Sampler/Sampling.cpp
	Sampler/RNG.h
	Sampler/LowDiscrepancy.h
	Sampler/LowDiscrepancy.cpp
	Sampler/SobolMatrices.h
	Sampler/SobolMatrices.cpp
	Sampler/Sampler.h
	Sampler/Sampler.cpp
	Sampler/halton.h
	Sampler/halton.cpp
	Sampler/clockRand.h
	Sampler/clockRand.cpp
)
# Make the Sampler group
SOURCE_GROUP("Sampler" FILES ${Sampler})

set(Integrator
	Integrator/Integrator.h
	Integrator/Integrator.cpp
	Integrator/WhittedIntegrator.cpp
	Integrator/WhittedIntegrator.h
)
# Make the Integrator group
SOURCE_GROUP("Integrator" FILES ${Integrator})

# Material files
set(Material
	Material/Reflection.h
	Material/Reflection.cpp
	Material/Material.h
	Material/Material.cpp
	Material/MatteMaterial.h
	Material/MatteMaterial.cpp
	Material/Fresnel.h
	Material/Fresnel.cpp
	Material/Mirror.h
	Material/Mirror.cpp
)
# Make the Material group
SOURCE_GROUP("Material" FILES ${Material})

set(Texture
	Texture/Texture.h
	Texture/Texture.cpp
	Texture/ConstantTexture.h
	Texture/ConstantTexture.cpp
)
# Make the Texture group
SOURCE_GROUP("Texture" FILES ${Texture})

# Light files
set(Light
	Light/Light.h
	Light/Light.cpp
	Light/PointLight.h
	Light/PointLight.cpp
	Light/DiffuseLight.h
	Light/DiffuseLight.cpp
	Light/SkyBoxLight.h
	Light/SkyBoxLight.cpp
)
# Make the Light group
SOURCE_GROUP("Light" FILES ${Light})

# Create executable
add_executable(PBR
	${Main}
	${Core}
	${Shape}
	${Accelerator}
	${Camera}
	${Sampler}
	${Integrator}
	${Material}
	${Texture}
	${Light}
 )

target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT PBR)




