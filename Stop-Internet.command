#!/bin/zsh
set -eu
job="gui/$(id -u)/com.classicchat.internet"
launchctl disable "$job"
if launchctl print "$job" >/dev/null 2>&1; then launchctl bootout "$job"; fi
echo 'Classic Chat internet is gestopt. Start-Internet.command zet het opnieuw aan.'
