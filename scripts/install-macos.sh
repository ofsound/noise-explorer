#!/bin/sh
# Install the already-built local development app and plugins for this user.
set -eu
project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=${1:-"$project_root/Builds/Release"}
configuration=${2:-Release}
artifacts="$build_dir/NoiseExplorer_artefacts/$configuration"

if [ "$(uname -s)" != Darwin ]; then
    echo 'This installer is for macOS.' >&2
    exit 1
fi
for bundle in "Standalone/Noise Explorer.app" "AU/Noise Explorer.component" "VST3/Noise Explorer.vst3"; do
    /usr/bin/codesign --verify --deep --strict "$artifacts/$bundle"
done
mkdir -p "$HOME/Applications" "$HOME/Library/Audio/Plug-Ins/Components" "$HOME/Library/Audio/Plug-Ins/VST3"
/usr/bin/ditto "$artifacts/Standalone/Noise Explorer.app" "$HOME/Applications/Noise Explorer.app"
/usr/bin/ditto "$artifacts/AU/Noise Explorer.component" "$HOME/Library/Audio/Plug-Ins/Components/Noise Explorer.component"
/usr/bin/ditto "$artifacts/VST3/Noise Explorer.vst3" "$HOME/Library/Audio/Plug-Ins/VST3/Noise Explorer.vst3"
printf 'Installed Noise Explorer %s: standalone in ~/Applications, AU and VST3 in ~/Library/Audio/Plug-Ins.\n' "$configuration"
