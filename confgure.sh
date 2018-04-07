#!/bin/bash
# This scripts sets up a new project ready to build and run.
set -eu

if [ ! -d ".git" ]; then
	git init .
fi

if [ ! -d "third_party" ]; then
	mkdir -p "third_party"
	pushd "third_party"

	# git submodule add git@github.com:cbeck88/visit_struct.git
	git submodule add git@github.com:emilk/emath.git
	git submodule add git@github.com:emilk/emilib.git
	git submodule add git@github.com:nothings/stb.git
	git submodule add git@github.com:ocornut/imgui.git

	pushd imgui
	git checkout v1.53
	popd

	popd
fi

git submodule update --init --recursive

if [ ! -d "src" ]; then
	mkdir -p "src"
	touch "src/libs.cpp" # TODO: get from some gist
	touch "src/main.cpp" # TODO: get from some gist
fi

# TODO: get buid.sh from some gist
