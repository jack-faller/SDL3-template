#!/bin/sh
set -exo pipefail

cd "$(dirname "$(which $0)")"
SELF="$(basename "$0")"

NAME="$(grep '^project(.*)$' CMakeLists.txt | sed 's/^project(\(.*\))$/\1/')"
while [ $# != 0 ]; do
	case "$1" in
		web)
			RELEASE="--debug"
			if [ "$2" = --release ]; then
				shift
				RELEASE="--release"
			fi
			docker run --rm --network=host --interactive --tty --volume "$(pwd):/proj" --user $UID:$GID emscripten/emsdk \
				   "/proj/$SELF" generic emscripten "$RELEASE"
			;;
		glad)
			glad --api='gles2=3.0' --out-path=libraries/glad
			;;
		linux)
			RELEASE="--debug"
			if [ "$2" = --release ]; then
				shift
				RELEASE="--release"
			fi
			./"$SELF" generic linux "$RELEASE"
			;;
		generic)
			TARGET="$2"
			TYPE="$3"
			shift 2
			RELEASE=-DCMAKE_BUILD_TYPE=Debug
			DIR="$TARGET"-build
			if [ "$TYPE" = --release ]; then
				shift
				RELEASE=-DCMAKE_BUILD_TYPE=Release
				DIR="$TARGET"-release
			fi
			if ! [ -e "$DIR" ]; then
				if [ "$TARGET" = emscripten ]; then
					emcmake cmake "$RELEASE" -S . -B "$DIR"
				else
					cmake "$RELEASE" -S . -B "$DIR"
				fi
			fi
			cmake --build "$DIR" --parallel "$(nproc)"
			;;
		run)
			shift
			exec "./linux-build/$NAME" "$@"
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
