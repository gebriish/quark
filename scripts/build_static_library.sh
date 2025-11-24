#!/usr/bin/env bash
set -e

echo "Building GLFW..."

cd "./src/thirdparty/glfw"
mkdir -p build
cd build

cmake .. \
	-DBUILD_SHARED_LIBS=OFF \
	-DGLFW_BUILD_DOCS=OFF \
	-DGLFW_BUILD_TESTS=OFF \
	-DGLFW_BUILD_EXAMPLES=OFF

cmake --build . --config Release
