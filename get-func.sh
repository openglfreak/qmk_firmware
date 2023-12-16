#!/bin/sh
# Copyright (C) 2022 Torge Matthies
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.
#
# Author contact info:
#   E-Mail address: openglfreak@googlemail.com
#   PGP key fingerprint: 0535 3830 2F11 C888 9032 FAD2 7C95 CD70 C9E8 438D

set -e 2>/dev/null ||:
set +C 2>/dev/null ||:
set +f 2>/dev/null ||:
set -u 2>/dev/null ||:

# zsh: Force word splitting.
setopt SH_WORD_SPLIT 2>/dev/null ||:
# zsh: Don't exit when a glob doesn't match.
unsetopt NOMATCH 2>/dev/null ||:

# description:
#   Escapes a string for usage in a sed pattern.
#   Sed expression copied from https://stackoverflow.com/a/2705678
# params:
#   [literal]: string
#     The string to escape. If omitted it's read from stdin
#   [separator]: char
#     The separator char. Defaults to a / (slash)
# outputs:
#   The escaped string
sed_escape_pattern() {
    if [ $# -gt 2 ]; then
        echo 'sed_escape_pattern: Too many arguments' >&2
        return 1
    fi
    if [ $# -ge 1 ]; then
        # Shellcheck bug.
        # shellcheck disable=SC2221,SC2222
        case "${2:-}" in
           ??*)
               echo 'sed_escape_pattern: Separator too long' >&2
               return 2;;
           []\\\$\*\.^[]) set -- "$1" '';;
           ''|*) set -- "$1" "${2:-/}"
        esac
        # shellcheck disable=SC1003
        printf '%s\n' "$1" | sed -e 's/[]\\'"$2"'$*.^[]/\\&/g' -e '$!s/$/\\/'
    else
        sed -e 's/[]\\/$*.^[]/\\&/g' -e '$!s/$/\\/'
    fi
}

extract_function() {
    if [ $# -ne 1 ]; then
        if [ $# -lt 1 ]; then
            echo 'extract_function: Not enough arguments' >&2
            return 1
        else
            echo 'extract_function: Too many arguments' >&2
            return 2
        fi
    fi
    set -- "$(sed_escape_pattern "$1")" || return
    sed -n "/^[0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f] <$1\.\{0,\}.*>:\$/,/^[0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f] <.*>:\$/p" | \
    head -n -1
}

postprocess_asm() {
    sed 's/^ [0-9A-Fa-f][0-9A-Fa-f]*:\t[0-9A-Fa-f][0-9A-Fa-f]*\( [0-9A-Fa-f][0-9A-Fa-f]*\)* *\t//'
}

#qmk compile >$(tty) && \
arm-none-eabi-objdump -D .build/keychron_q3_iso_encoder_openglfreak.elf | \
extract_function "$1" #| \
#postprocess_asm
