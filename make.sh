#!/bin/sh
sh -c 'set -o pipefail' && set -o pipefail
set -ex

cd "$(dirname "$(which $0)")"
SELF="$(basename "$0")"
NL="
"

docker_run () {
	docker run --rm --network=host --interactive --tty --volume "$(pwd):/app" --workdir /app --user $UID:$GID emscripten/emsdk "$@"
}

NAME="$(grep '^project(.*)$' CMakeLists.txt | sed 's/^project(\(.*\))$/\1/')"
while [ $# != 0 ]; do
	case "$1" in
		clean)
			rm -rf linux-debug linux-release emscripten-debug emscripten-release
			;;
		web)
			RELEASE="--debug"
			if [ "$2" = --release ]; then
				shift
				RELEASE="--release"
			fi
			docker_run "/app/$SELF" generic emscripten "$RELEASE"
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
			DIR="$TARGET"-debug
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
			if [ "$1" = --release ]; then
				shift
				exec "./linux-release/Release/$NAME" "$@"
			else
				exec "./linux-debug/Debug/$NAME" "$@"
			fi
			;;
		pack)
			DIR=emscripten-debug/Debug
			if [ "$2" = --release ]; then
				shift
				DIR=emscripten-release/Release
			fi
			gcc tools/base128-encode.c -o emscripten-debug/base128-encode
			cp "src/shell.html" "$DIR/$NAME-pack.html"
			echo -n '[' > "$DIR/manifest.json"
			rm "$DIR/files.bin" || true
			SEPP=""
			add_pack_file() {
				TYPE="$1"
				LENGTH="$(wc --bytes < $2)"
				FNAME="$(echo ${3-$2} | sed "s/'/\\'/g")"
				echo -n "$SEPP{\"name\":\"$FNAME\",\"type\":\"$TYPE\",\"length\":$LENGTH}" >> "$DIR/manifest.json"
				SEPP=,
				cat "$2" >> "$DIR/files.bin"
			}
			add_pack_file application/wasm "$DIR/$NAME.wasm" "$NAME.wasm"
			add_pack_file application/octet-stream "$DIR/$NAME.data" "$NAME.data"
			add_pack_file application/javascript "$DIR/$NAME.js" "script.js"
			echo -n ']' >> "$DIR/manifest.json"
			(echo -n "string:'"
			 cat "$DIR/manifest.json" "$DIR/files.bin" \
				 | gzip --to-stdout - | ./emscripten-debug/base128-encode
			 echo -n "',manifest_length:$(wc --bytes < "$DIR/manifest.json")"
			) | sed "/!! DATA_HERE !!/r /dev/stdin" -i "$DIR/$NAME-pack.html"
			sed '/{{{ SCRIPT }}}/d' -i "$DIR/$NAME-pack.html"
			;;
		*)
			echo "Unknown target \"$1\""
			exit 1
			;;
	esac
	shift
done
