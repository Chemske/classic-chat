#!/bin/zsh
set -eu
job="gui/$(id -u)/com.classicchat.internet"
launchctl enable "$job"
if launchctl print "$job" >/dev/null 2>&1; then
  echo 'Classic Chat internet draait al. Het adres staat in Internetadres.txt.'
else
  project_dir="${0:A:h}"
  runtime_dir="$HOME/Library/Application Support/ClassicChat/server"
  cp "$project_dir/server/server.mjs" "$runtime_dir/server.mjs"
  cp "$project_dir/server/remote-service.mjs" "$runtime_dir/remote-service.mjs"
  launchctl bootstrap "gui/$(id -u)" "$HOME/Library/LaunchAgents/com.classicchat.internet.plist"
  echo 'Classic Chat internet gestart. Het actuele adres verschijnt in Internetadres.txt.'
fi
