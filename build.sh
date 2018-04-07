#!/bin/bash
set -eu

if [ ! -z ${1+x} ] && [ $1 == "clean" ]; then
	rm -rf build/
	rm -f *.bin
	exit 0
fi

mkdir -p build

CXX="ccache g++"
CPPFLAGS="--std=c++14"
# CPPFLAGS="--std=c++1z" # C++17

CPPFLAGS="$CPPFLAGS -Werror -Wall -Wpedantic -Wextra -Wunreachable-code"

CPPFLAGS="$CPPFLAGS -Wno-double-promotion" # Implicitly converting a float to a double is fine
CPPFLAGS="$CPPFLAGS -Wno-shorten-64-to-32"
CPPFLAGS="$CPPFLAGS -Wno-sign-compare"
CPPFLAGS="$CPPFLAGS -Wno-sign-conversion"

# TEMPORARY:
# CPPFLAGS="$CPPFLAGS -Wno-unused-function"
# CPPFLAGS="$CPPFLAGS -Wno-unused-parameter"
# CPPFLAGS="$CPPFLAGS -Wno-unused-variable"

# Check if clang:ret=0
ret=0
$CXX --version 2>/dev/null | grep clang > /dev/null || ret=$?
if [ $ret == 0 ]; then
	# Clang:
	CPPFLAGS="$CPPFLAGS -Wno-gnu-zero-variadic-macro-arguments" # Loguru
	CPPFLAGS="$CPPFLAGS -Wno-#warnings" # emilib
else
	# GCC:
	CPPFLAGS="$CPPFLAGS -Wno-maybe-uninitialized" # stb
fi

CPPFLAGS="$CPPFLAGS -O2 -DNDEBUG" # RELEASE BUILD
# CPPFLAGS="$CPPFLAGS -g -fsanitize=address -fno-omit-frame-pointer" # DEBUG BUILD

COMPILE_FLAGS="$CPPFLAGS"
COMPILE_FLAGS="$COMPILE_FLAGS -I ."
COMPILE_FLAGS="$COMPILE_FLAGS -isystem third_party"
COMPILE_FLAGS="$COMPILE_FLAGS -isystem third_party/emath"
COMPILE_FLAGS="$COMPILE_FLAGS -isystem third_party/emilib"
COMPILE_FLAGS="$COMPILE_FLAGS -isystem third_party/visit_struct/include"
LDLIBS="-lstdc++ -lpthread -ldl"
LDLIBS="$LDLIBS -lSDL2 -lGLEW"
# LDLIBS="$LDLIBS -lceres -lglog"
LDLIBS="$LDLIBS -lfftw3f"

# Platform specific flags:
if [ "$(uname)" == "Darwin" ]; then
	COMPILE_FLAGS="$COMPILE_FLAGS -isystem /opt/local/include/eigen3" # port
	COMPILE_FLAGS="$COMPILE_FLAGS -isystem /usr/local/include/eigen3" # brew
	LDLIBS="$LDLIBS -framework OpenAL"
	LDLIBS="$LDLIBS -framework OpenGL"
else
	COMPILE_FLAGS="$COMPILE_FLAGS -isystem /usr/include/eigen3"
	LDLIBS="$LDLIBS -lGL -lkqueue"
fi

# COMPILE_FLAGS="$COMPILE_FLAGS -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=1"

echo "Compiling..."
OBJECTS=""
for source_path in src/*.cpp; do
	rel_source_path=${source_path#src/} # Remove src/ path prefix
	obj_path="build/${rel_source_path%.*}.o" # Strip file extension
	OBJECTS="$OBJECTS $obj_path"
	rm -f $obj_path
	$CXX $COMPILE_FLAGS -c $source_path -o $obj_path &
done

wait

echo >&2 "Linking..."
FOLDER_NAME=${PWD##*/}
$CXX $CPPFLAGS $OBJECTS $LDLIBS -o "$FOLDER_NAME.bin"
