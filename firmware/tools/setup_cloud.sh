#!/usr/bin/env bash
set -euo pipefail
# User-space installation: no sudo, no changes to /etc or tracked sources.
avr_setup=/workspace/.avr
repo_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
case "$(uname -m)" in x86_64) ;; *) echo 'This setup script targets Debian amd64.' >&2; exit 1;; esac
mkdir -p "$avr_setup/apt/lists/partial" "$avr_setup/apt/archives/partial" "$avr_setup/packages" "$avr_setup/root" "$avr_setup/apt/empty"
if [[ "${AVR_REFRESH:-0}" == 1 || ! -x "$avr_setup/root/usr/bin/avr-gcc" || ! -e "$avr_setup/root/usr/include/libelf.h" ]]; then
    cat > "$avr_setup/apt/sources.list" <<'EOF'
deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] https://deb.debian.org/debian trixie main
EOF
    cat > "$avr_setup/apt/config" <<EOF
Dir::Etc::parts "$avr_setup/apt/empty";
Dir::Etc::main "/dev/null";
Dir::Etc::sourcelist "$avr_setup/apt/sources.list";
Dir::Etc::sourceparts "$avr_setup/apt/empty";
Dir::State::lists "$avr_setup/apt/lists";
Dir::Cache::archives "$avr_setup/apt/archives";
APT::Sandbox::User "$(id -un)";
APT::Update::Error-Mode "any";
Acquire::Retries "0";
Acquire::Languages "none";
EOF
    APT_CONFIG="$avr_setup/apt/config" /usr/bin/apt-get update
    (
        cd "$avr_setup/packages"
        APT_CONFIG="$avr_setup/apt/config" /usr/bin/apt-get download \
            gcc-avr=1:14.2.0-2 binutils-avr=2.43.50.20250108-1 avr-libc=1:2.2.1-1 \
            libelf-dev=0.192-4 libelf1t64=0.192-4
        for package in gcc-avr_*14.2.0-2_*.deb binutils-avr_*2.43.50.20250108-1_*.deb \
            avr-libc_*2.2.1-1_*.deb libelf-dev_0.192-4_*.deb libelf1t64_0.192-4_*.deb; do
            dpkg-deb -x "$package" "$avr_setup/root"
        done
    )
fi
export PATH="$avr_setup/root/usr/bin:$PATH"
sim_commit=f44723e8c42431136d5b4de81f789ded56d7e8fa # upstream simavr v1.8
if [[ ! -d "$avr_setup/simavr-source/.git" ]]; then
    git clone --depth 1 --branch v1.8 https://github.com/buserror/simavr.git "$avr_setup/simavr-source"
fi
actual_commit=$(git -C "$avr_setup/simavr-source" rev-parse HEAD)
[[ "$actual_commit" == "$sim_commit" ]] || { echo 'Unexpected simavr checkout; preserve it and investigate.' >&2; exit 1; }
export PKG_CONFIG_PATH="$avr_setup/root/usr/lib/x86_64-linux-gnu/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export CPATH="$avr_setup/root/usr/include${CPATH:+:$CPATH}"
export LIBRARY_PATH="$avr_setup/root/usr/lib/x86_64-linux-gnu${LIBRARY_PATH:+:$LIBRARY_PATH}"
export LD_LIBRARY_PATH="$avr_setup/root/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
pkg-config --exists libelf
make -C "$avr_setup/simavr-source/simavr" -j2
cat > "$avr_setup/activate.sh" <<'EOF'
export PATH="/workspace/.avr/root/usr/bin:$PATH"
export LD_LIBRARY_PATH="/workspace/.avr/simavr-source/simavr/obj-x86_64-linux-gnu:/workspace/.avr/root/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export SIM_CFLAGS="-I/workspace/.avr/simavr-source/simavr/sim"
export SIM_LDFLAGS="-L/workspace/.avr/simavr-source/simavr/obj-x86_64-linux-gnu"
EOF
source "$avr_setup/activate.sh"
avr-gcc --version
make -C "$repo_dir/firmware" -j2 matrix
make -C "$repo_dir/firmware" test
make -C "$repo_dir/firmware" PROFILE=base simulate
make -C "$repo_dir/firmware" PROFILE=full simulate
