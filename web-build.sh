#!/bin/sh
docker run --rm --network=host --interactive --tty --volume "$(pwd):/proj" --workdir /proj --user $UID:$GID emscripten/emsdk sh -c 'emcmake cmake -S . -B emscripten-build && cd emscripten-build && emmake make'
