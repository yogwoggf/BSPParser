cmake_minimum_required(VERSION 3.20)

project("BSPParser" CXX)
set(CMAKE_CXX_STANDARD 20)

add_library(
	${PROJECT_NAME}
	"BSPParser.cpp"

	"FileFormat/Parser.cpp"
	"FileFormat/Vector.cpp"

	"Displacements/Displacements.cpp"
	"Displacements/UVGen.cpp"
	"Displacements/TBNGen.cpp"
	"Displacements/NormalBlending.cpp"
	"Displacements/SubEdgeIterator.cpp"

	"Utility/ErrorHandling.h"
)

target_include_directories(
	${PROJECT_NAME} PRIVATE

	${PROJECT_SOURCE_DIR}
)
