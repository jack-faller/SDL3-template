#!/bin/sh
set -exo pipefail

cd "$(dirname "$(which $0)")"

while [ $# != 0 ]; do
	case "$1" in
		web)
			docker run --rm --network=host --interactive --tty --volume "$(pwd):/proj" --workdir /proj --user $UID:$GID emscripten/emsdk sh -c 'emcmake cmake -S . -B emscripten-build && cd emscripten-build && emmake make'
			;;
		build)
			if ! [ -e build ]; then
				cmake -S . -B build
			fi
			cmake --build build
			;;
		run)
			shift
			NAME="$(grep '^project(.*)$' CMakeLists.txt | sed 's/^project(\(.*\))$/\1/')"
			"./build/$NAME" "$@"
			exit
	esac
	shift
done
