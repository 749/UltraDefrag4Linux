#!/usr/bin/env bash
set -euo pipefail

binary="${1:-./udefrag}"
volume_size_mb="${VOLUME_SIZE_MB:-1024}"
filler_file_count="${FILLER_FILE_COUNT:-300}"
filler_file_size_mb="${FILLER_FILE_SIZE_MB:-3}"
fragmented_file_count="${FRAGMENTED_FILE_COUNT:-40}"
fragmented_file_size_mb="${FRAGMENTED_FILE_SIZE_MB:-5}"
supported_filesystems="${SUPPORTED_FILESYSTEMS:-ntfs}"

if [[ "$(id -u)" -ne 0 ]]; then
    echo "This test creates and mounts a temporary NTFS image; run it as root." >&2
    exit 1
fi

if [[ ! -x "$binary" ]]; then
    echo "Binary not found or not executable: $binary" >&2
    exit 1
fi

for tool in mkntfs ntfs-3g ntfsfix umount dd truncate; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Required tool is missing: $tool" >&2
        exit 1
    fi
done

workdir="$(mktemp -d "${TMPDIR:-/tmp}/udefrag-fragmented-volume.XXXXXX")"
mountpoint="$workdir/mnt"
mounted=0

cleanup() {
    if [[ "$mounted" -eq 1 ]]; then
        sync || true
        umount "$mountpoint" || true
    fi
    rm -rf "$workdir"
}
trap cleanup EXIT

step() {
    printf '==> %s\n' "$*"
}

run() {
    printf '+'
    printf ' %q' "$@"
    printf '\n'
    "$@"
}

create_file() {
    local path="$1"
    local size_mb="$2"

    dd if=/dev/zero of="$path" bs=1M count="$size_mb" status=none conv=fsync
}

run_ntfs_case() {
    local image="$workdir/udefrag-fragmented-volume-ntfs.img"
    local filler_root
    local fragment_root

    step "Creating temporary ${volume_size_mb}MB NTFS image at $image"
    mkdir -p "$mountpoint"
    run truncate -s "${volume_size_mb}M" "$image"
    run mkntfs -F -Q -L UDFRAGCI "$image"

    step "Checking fresh NTFS image"
    run ntfsfix -n "$image"

    step "Mounting NTFS image"
    run ntfs-3g "$image" "$mountpoint"
    mounted=1

    filler_root="$mountpoint/udefrag-ci/filler"
    fragment_root="$mountpoint/udefrag-ci/fragmented"
    mkdir -p "$filler_root" "$fragment_root"

    step "Creating allocation pressure files"
    for ((i = 0; i < filler_file_count; i++)); do
        create_file "$filler_root/filler-$(printf '%04d' "$i").bin" "$filler_file_size_mb"
    done

    step "Removing alternating files to create free-space holes"
    for ((i = 0; i < filler_file_count; i += 2)); do
        rm -f "$filler_root/filler-$(printf '%04d' "$i").bin"
    done

    step "Creating test files expected to span multiple holes"
    for ((i = 0; i < fragmented_file_count; i++)); do
        create_file "$fragment_root/fragmented-$(printf '%04d' "$i").bin" "$fragmented_file_size_mb"
    done

    sync
    step "Unmounting NTFS image before defragmenter run"
    run umount "$mountpoint"
    mounted=0

    step "Checking populated NTFS image"
    run ntfsfix -n "$image"

    step "Analyzing fragmented NTFS image"
    run "$binary" --analyze "$image"

    step "Running dry-run defragmentation against fragmented NTFS image"
    run env UD_DRY_RUN=1 "$binary" --defragment "$image"

    step "Checking NTFS image after dry-run"
    run ntfsfix -n "$image"
}

for fs in $supported_filesystems; do
    case "${fs,,}" in
        ntfs)
            step "Starting filesystem case: NTFS"
            run_ntfs_case
            ;;
        *)
            echo "Unsupported Linux filesystem test case: $fs" >&2
            echo "The Linux build currently supports NTFS device images only." >&2
            exit 1
            ;;
    esac
done
