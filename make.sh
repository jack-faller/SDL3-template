#!/bin/sh
set -exo pipefail

cd "$(dirname "$(which $0)")"

while [ $# != 0 ]; do
	case "$1" in
		web)
			docker run --rm --network=host --interactive --tty --volume "$(pwd):/proj" --workdir /proj --user $UID:$GID emscripten/emsdk sh -c 'emcmake cmake -S . -B emscripten-build && cd emscripten-build && emmake make --jobs "$(nproc)"'
			;;
		glad)
			glad --api='gles2=3.0' --out-path=libraries/glad
			;;
		linux)
			if ! [ -e linxu-build ]; then
				cmake -S . -B linux-build
			fi
			cmake --build linux-build --parallel "$(nproc)"
			;;
		run)
			shift
			NAME="$(grep '^project(.*)$' CMakeLists.txt | sed 's/^project(\(.*\))$/\1/')"
			"./linux-build/$NAME" "$@"
			exit
	esac
	shift
done
