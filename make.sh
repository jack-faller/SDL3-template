#!/bin/sh
set -exo pipefail

cd "$(dirname "$(which $0)")"

NAME="$(grep '^project(.*)$' CMakeLists.txt | sed 's/^project(\(.*\))$/\1/')"
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
			"./linux-build/$NAME" "$@"
			exit
			;;
		string-sub)
			REPLACE="$(grep WASM_STRING emscripten-build/$NAME.html)"
			gcc base128-encode.c -o emscripten-build/base128-encode
			(echo "$REPLACE" | sed 's/\(.*\)WASM_STRING.*/\1/' | tr -d '\n'
			 emscripten-build/base128-encode "emscripten-build/$NAME.wasm"
			 echo "$REPLACE" | sed 's/.*WASM_STRING\(.*\)/\1/' | tr -d '\n') | sed '/WASM_STRING/{r /dev/stdin
d }' "emscripten-build/$NAME.html" > "emscripten-build/$NAME-pack.html"
			;;
	esac
	shift
done
