#!/bin/bash
set -eu

if [ ! -z ${1+x} ] && [ $1 == "clean" ]; then
	rm -rf build/
	rm -f *.bin
	exit 0
fi

FOLDER_NAME=${PWD##*/}
BINARY_NAME="$FOLDER_NAME.bin"

mkdir -p build

OPENGL_REFERENCE_RENDERER=false # Set to true to try the OpenGL reference renderer

CXX="ccache g++"

# -----------------------------------------------------------------------------
# Flags for compiler and linker:

if $OPENGL_REFERENCE_RENDERER; then
	CPPFLAGS="--std=c++14"
else
	CPPFLAGS="--std=c++11"
fi

# CPPFLAGS="$CPPFLAGS -Werror -Wall -Wpedantic -Wextra -Weverything -Wunreachable-code" # These can all be turned on, but are off just to make the example compile everywhere.

CPPFLAGS="$CPPFLAGS -Wno-double-promotion" # Implicitly converting a float to a double is fine
CPPFLAGS="$CPPFLAGS -Wno-float-equal" # Comparing floating point numbers is fine if you know what you're doing
CPPFLAGS="$CPPFLAGS -Wno-shorten-64-to-32"
CPPFLAGS="$CPPFLAGS -Wno-sign-compare"
CPPFLAGS="$CPPFLAGS -Wno-sign-conversion"

# Turn off some warning that -Weverything turns on:
CPPFLAGS="$CPPFLAGS -Wno-c++98-compat"
CPPFLAGS="$CPPFLAGS -Wno-c++98-compat-pedantic"
# CPPFLAGS="$CPPFLAGS -Wno-covered-switch-default"
CPPFLAGS="$CPPFLAGS -Wno-disabled-macro-expansion"
CPPFLAGS="$CPPFLAGS -Wno-documentation"
CPPFLAGS="$CPPFLAGS -Wno-documentation-unknown-command"
CPPFLAGS="$CPPFLAGS -Wno-exit-time-destructors"
# CPPFLAGS="$CPPFLAGS -Wno-global-constructors"
# CPPFLAGS="$CPPFLAGS -Wno-missing-noreturn"
CPPFLAGS="$CPPFLAGS -Wno-missing-prototypes"
CPPFLAGS="$CPPFLAGS -Wno-padded"
CPPFLAGS="$CPPFLAGS -Wno-reserved-id-macro"
CPPFLAGS="$CPPFLAGS -Wno-unused-macros"

# CPPFLAGS="$CPPFLAGS -Wno-unused-function" # Useful during development (TEMPORARY)
# CPPFLAGS="$CPPFLAGS -Wno-unused-parameter" # Useful during development (TEMPORARY)
# CPPFLAGS="$CPPFLAGS -Wno-unused-variable" # Useful during development (TEMPORARY)

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

DEBUG_SYMBOLS="-g -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -fno-optimize-sibling-calls"

# CPPFLAGS="$CPPFLAGS $DEBUG_SYMBOLS -fsanitize=address" # Debug build with fsanatize
# CPPFLAGS="$CPPFLAGS $DEBUG_SYMBOLS"                    # Debug build
# CPPFLAGS="$CPPFLAGS -O2 -DNDEBUG $DEBUG_SYMBOLS"       # Release build with debug symbols
CPPFLAGS="$CPPFLAGS -O2 -DNDEBUG"                      # Release build

CPPFLAGS="$CPPFLAGS -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS"

# -----------------------------------------------------------------------------
# Flags for compiler:

COMPILE_FLAGS="$CPPFLAGS"
COMPILE_FLAGS="$COMPILE_FLAGS -I ."
COMPILE_FLAGS="$COMPILE_FLAGS -isystem third_party"
COMPILE_FLAGS="$COMPILE_FLAGS -isystem third_party/emilib"

# -----------------------------------------------------------------------------
# Custom compile-time flags:

COMPILE_FLAGS="$COMPILE_FLAGS -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=1"
COMPILE_FLAGS="$COMPILE_FLAGS -DLOGURU_REDEFINE_ASSERT=1"

# -----------------------------------------------------------------------------
# Libraries to link with:

LDLIBS="-lstdc++ -lpthread -ldl"
LDLIBS="$LDLIBS -lSDL2"

if $OPENGL_REFERENCE_RENDERER; then
	COMPILE_FLAGS="$COMPILE_FLAGS -DOPENGL_REFERENCE_RENDERER"
	LDLIBS="$LDLIBS -lGLEW"
	if [ "$(uname)" == "Darwin" ]; then
		LDLIBS="$LDLIBS -framework OpenGL"
	else
		LDLIBS="$LDLIBS -lGL"
	fi
fi

# -----------------------------------------------------------------------------

echo "Compiling..."
OBJECTS=""
for source_path in src/*.cpp; do
	rel_source_path=${source_path#src/} # Remove src/ path prefix
	obj_path="build/${rel_source_path}.o"
	OBJECTS="$OBJECTS $obj_path"
	rm -f $obj_path
	$CXX $COMPILE_FLAGS -c $source_path -o $obj_path &
done

wait

echo >&2 "Linking..."
$CXX $CPPFLAGS $OBJECTS $LDLIBS -o "$BINARY_NAME"

echo >&2 "Generating .dSYM..."
dsymutil "$BINARY_NAME" -o "$BINARY_NAME.dSYM"

echo >&2 "Build done."
