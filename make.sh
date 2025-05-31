#!/bin/sh
sh -c 'set -o pipefail' && set -o pipefail
set -ex

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
			if [ "$TARGET" = emscripten ]; then
				emcmake cmake "$RELEASE" -S . -B "$DIR"
				cd "$DIR"
				emmake make --jobs "$(nproc)"
				cd -
			else
				[ -e "$DIR" ] || cmake "$RELEASE" -S . -B "$DIR"
				cmake --build "$DIR" --parallel "$(nproc)"
			fi
			;;
		run)
			shift
			exec "./linux-build/$NAME" "$@"
			;;
		string-sub)
			gcc base128-encode.c -o emscripten-build/base128-encode
			(echo -n "	'$NAME.wasm': '"
			 ./emscripten-build/base128-encode "emscripten-build/$NAME.wasm"
			 echo -n "'"
			) | sed '/!! DATA_STRINGS !!/r /dev/stdin' "emscripten-build/$NAME.html" > "emscripten-build/$NAME-pack.html"
			;;
	esac
	shift
done
