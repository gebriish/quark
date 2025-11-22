#!/usr/bin/env bash
set -e

GLFW_VERSION="3.4"
GLFW_DIR="./src/thirdparty/glfw"
GLFW_URL="https://github.com/glfw/glfw/archive/refs/tags/${GLFW_VERSION}.tar.gz"

if [[ "$1" == "--fetch" ]]; then
	echo "Fetching GLFW ${GLFW_VERSION}..."

	mkdir -p ./src/thirdparty

		if [[ -d "$GLFW_DIR" ]]; then
			echo "Removing existing GLFW directory..."
			rm -rf "$GLFW_DIR"
		fi

		TMPFILE=$(mktemp)
		echo "Downloading $GLFW_URL ..."
		curl -L "$GLFW_URL" -o "$TMPFILE"

		tar -xzf "$TMPFILE" -C ./src/thirdparty
		rm "$TMPFILE"

		mv "./src/thirdparty/glfw-${GLFW_VERSION}" "$GLFW_DIR"

		echo "GLFW ${GLFW_VERSION} downloaded into ${GLFW_DIR}"
		exit 0
fi

# build

echo "Building GLFW..."

cd "$GLFW_DIR"
mkdir -p build
cd build

cmake .. \
	-DBUILD_SHARED_LIBS=OFF \
	-DGLFW_BUILD_DOCS=OFF \
	-DGLFW_BUILD_TESTS=OFF \
	-DGLFW_BUILD_EXAMPLES=OFF

cmake --build . --config Release
