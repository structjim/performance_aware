#!/bin/bash

###################
## BUILD AND RUN ##
###################

echo "Argument 1 for BAR.sh is: "
echo $1
echo " "

#c source file, sans extension
disassembler="hw03"
#inASM is the .asm passed to this .bat as an argument
inASM=$1

#Assemble input asm to bin
nasm $inASM -o assembled_input_bin

#Compile disassembler
gcc $disassembler.c -o $disassembler
#Run disassembled (Params are input bin, output asm)
./$disassembler assembled_input_bin disassembled_output.asm

echo .
echo [Re-assembling for binary comparison...]
echo .

#RE-assemble
nasm disassembled_output.asm -o REassembled_input

#Compare
echo /\\Comparing original bin with re-assembled bin...
diff assembled_input_bin REassembled_input
echo \\/Comparison over.

#Clean up output
rm assembled_input_bin
rm disassembled_output.asm
rm REassembled_input

#END
