#!/usr/bin/env bash

make -C build
make install -C build

homedir="$HOME"

if ! grep -Pi "#dbgecko" ~/.zshrc ; then
	echo "export PATH=\"\$PATH:$(cd `dirname $0` && pwd)/bin\" #dbgecko" >> "$homedir/.zshrc"

	if type -P zsh > /dev/null 2>&1; then # zshell support
		echo "source $homedir/.zshrc" | zsh
	fi
fi

