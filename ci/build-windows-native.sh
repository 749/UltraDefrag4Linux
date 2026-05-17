#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

export BUILD_ENV=mingw_x64
export COMPILER_BIN=/ucrt64/bin/
export UDEFRAG_LIB_PATH=../../lib/amd64

# mkmod.lua invokes gmake for x64 MinGW builds. MSYS2 provides make.
mkdir -p ci/bin src/bin/amd64 src/lib/amd64
cat > ci/bin/gmake <<'EOF'
#!/usr/bin/env sh
exec make "$@"
EOF
chmod +x ci/bin/gmake
export PATH="$repo_root/ci/bin:$PATH"

# The legacy Windows build staged getopt sources before generating the
# console makefile.
cp src/share/getopt.c src/share/getopt1.c src/share/getopt.h src/console/

build_module() {
    local module_dir="$1"
    local build_file="$2"

    (
        cd "$module_dir"
        lua ../../tools/mkmod.lua "$build_file"
    )
}

(
    cd src/lua5.1
    lua ../tools/mkmod.lua lua5.1a_dll.build
)

build_module src/dll/wgx wgx.build
build_module src/dll/zenwinx zenwinx.build
build_module src/dll/udefrag udefrag.build

(
    cd src/console
    lua ../tools/mkmod.lua defrag.build
)

test -f src/bin/amd64/udefrag.exe
file src/bin/amd64/udefrag.exe
