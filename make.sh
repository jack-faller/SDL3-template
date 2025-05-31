#!/bin/sh
sh -c 'set -o pipefail' && set -o pipefail
set -ex

cd "$(dirname "$(which $0)")"
SELF="$(basename "$0")"
NL="
"

docker_run () {
	docker run --rm --network=host --interactive --tty --volume "$(pwd):/proj" --workdir /proj --user $UID:$GID emscripten/emsdk "$@"
}

NAME="$(grep '^project(.*)$' CMakeLists.txt | sed 's/^project(\(.*\))$/\1/')"
while [ $# != 0 ]; do
	case "$1" in
		web)
			RELEASE="--debug"
			if [ "$2" = --release ]; then
				shift
				RELEASE="--release"
			fi
			docker_run "/proj/$SELF" generic emscripten "$RELEASE"
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
		pack)
			DIR=emscripten-build/Debug
			if [ "$2" = --release ]; then
				shift
				DIR=emscripten-release/Release
			fi
			gcc tools/base128-encode.c -o emscripten-build/base128-encode
			cp "src/shell.html" "$DIR/$NAME-pack.html"
			add_pack_file() {
				(echo -n "	{ name: '${3-$2}', type: '$1', text: '"
				 gzip --to-stdout "$DIR/$2" | ./emscripten-build/base128-encode
				 echo "' },"
				) | sed '/!! DATA_STRINGS !!/r /dev/stdin' -i "$DIR/$NAME-pack.html"
			}
			add_pack_file application/wasm "$NAME.wasm"
			add_pack_file application/octet-stream "$NAME.data"
			add_pack_file application/javascript "$NAME.js" "script.js"
			sed '/{{{ SCRIPT }}}/d' -i "$DIR/$NAME-pack.html"
			;;
	esac
	shift
done
