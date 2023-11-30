# HackAssembler
Hack Assembler in C++

Part of the assignment for "From Nand to Tetris" by Hebrew University of Jerusalem at coursera.org

This program assembles the instruction from Hack assembly language into binary codes stored in `.hack` files. More details about the specifications of the proposed architecture can be found on the nand2tetris project site, https://www.nand2tetris.org/project06

## How it works

The program consists of two parts, label replacement and binary conversion. 
Label replacement would read the file twice, one to identify the location of labels inside the source code, and replace it with the their locations.

Binary conversion would involved reading the cleaned assembly instruction from the previous step, and assemble it to binary based on the specifications.

## To run

`g++ assembler.cpp -o test` 

To change the files you want to assemble and save as, you have to edit `writeFileLocation` and `inputFile` field in line 665 and 666.
