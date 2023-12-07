#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>
#include <bitset>
#include <cstdint>
#include <algorithm>
 
// Hack Assembler
//
// LoadInstructionFromFile
// Input : file
// Process :
//  2 passes
//   1st pass
//    Sanitize, get the labels,
//    put it in the symbol table along with the address,
//    also put all the valid instruction in the list
//   2nd pass
//    Go through the sanitized instruction in the list,
//    replace labels with exact number/address
// Output :
//  a list of cleaned instructions (vector)
 
// AssembleInstruction
// Input :
//  a list of cleaned instructions (vector)
// Process :
//  check what command is it
//  for a-command
//   if first char is @, then it's a-command
//    to convert to binary
//     convert the value to binary
//  
//  for c-instruction
//   split to 3 parts
//   remember the format of the instruction
//   dest = comp ; jump
//   1 1 1 a c1 c2 c4 c5 c5 c6 d1 d2 d3 j1 j2 j3
//      <------comp-------&gt; <--des-> <-jump->
//   either the dest or jump field may be empty
//    if dest is empty, no =
//    if jump is empty, no ;
//   for comp
//    have both, so comp is somewhere in between dest and jump
//     dest = comp ; jump
//    dest, comp would be after =
//     dest = comp
//    jump, comp would be before ;
//     comp ; jump
//    only comp
//     comp
//   identify comp
//    no easy way to get around this, so
//    compare each comp to the table, and put the equivalent binary in the list
 
/*
-----------------------------------------
 | comp  | c1 c2 c3 c4 c5 c6 |
 ----------------------------------------
 | 0 |  | 1  0  1  0  1  0 |
 | 1 |  | 1  1  1  1  1  1 |
 | -1 |  |   1  1  1  0  1  0 |
 | D |  |   0  0  1  1  0  0 |
 | A | M |   1  1  0  0  0  0 |
 | !D |  |   0  0  1  1  0  1 |
 | !A | !M | 1  1  0  0  0  1 |
 | -D |  | 0  0  1  1  1  1 |
 | -A |  | 1  1  0  0  1  1 |
 | D+1 |  | 0  1  1  1  1  1 |
 | A+1 | M+1 | 1  1  0  1  1  1 |
 | D-1 |  | 0  0  1  1  1  0 |
 | A-1 | M-1 | 1  1  0  0  1  0 |
 | D+A | D+M | 0  0  0  0  1  0 |
 | D-A | D-M | 0  1  0  0  1  1 |
 | A-D | M-D | 0  0  0  1  1  1 |
 | D&A | D&M | 0  0  0  0  0  0 |
 | D|A | D|M | 0  1  0  1  0  1 |
-----------------------------------------
 | a=0 | a=1 |      |
 ----------------------------------------
 
 */
 // Text processing
 // Doesn't actually handles multiple delimiters, bummer
 // https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
 
 
 // somehow <regex> defines it's own remove_if, which breaks other things
 // https://stackoverflow.com/a/58164098
 
 // Doesn't handle special cases nicely
 // https://stackoverflow.com/questions/7621727/split-a-string-into-words-by-multiple-delimiters
 
 
 // Input : string to be split
 // Process :
 //  split the string into multiple string
 //  ignore everything past // and itself (comments)
 //  combine the string together
 // Output : a single string, stripped of whitespace
std::string SanitizeIns(std::string input)
{
 std::stringstream stringStream(input);
 std::stringstream output;
 std::string token;
 
 while (stringStream >> token)
 {
  if (token.substr(0, 2) == "//")
  {
   break;
  }
  else
  {
   output << token;
  }
 }
 return output.str();
}
 
// LoadInstructionFromFile
// Input : file
// Process :
//  2 passes
//   1st pass
//    Sanitize, get the labels,
//    put it in the symbol table along with the address,
//    also put all the valid instruction in the list
//   2nd pass
//    Go through the sanitized instruction in the list,
//    replace labels with exact number/address
// Output :
//  a list of cleaned instructions (vector)
void LoadInstructionFromFile(std::vector<std::string>& instructionList, std::string inputFile)
{
 // Initialization
 instructionList.clear();
 
 // Files
 std::ifstream asmFile;
 asmFile.open(inputFile);
 
 // String buffer
 std::string currentLine;
 
 // Symbol Table
 std::map<std::string, int> symbolTable;
 
 // add predefined symbols
 symbolTable.insert( {"SP", 0x0});
 symbolTable.insert( {"LCL", 0x1});
 symbolTable.insert( {"ARG", 0x2});
 symbolTable.insert( {"THIS", 0x3});
 symbolTable.insert( {"THAT", 0x4});
 
 symbolTable.insert( {"R0", 0x0});
 symbolTable.insert( {"R1", 0x1});
 symbolTable.insert( {"R2", 0x2});
 symbolTable.insert( {"R3", 0x3});
 symbolTable.insert( {"R4", 0x4});
 symbolTable.insert( {"R5", 0x5});
 symbolTable.insert( {"R6", 0x6});
 symbolTable.insert( {"R7", 0x7});
 symbolTable.insert( {"R8", 0x8});
 symbolTable.insert( {"R9", 0x9});
 symbolTable.insert( {"R10", 0xA});
 symbolTable.insert( {"R11", 0xB});
 symbolTable.insert( {"R12", 0xC});
 symbolTable.insert( {"R13", 0xD});
 symbolTable.insert( {"R14", 0xE});
 symbolTable.insert( {"R15", 0xF});
 
 symbolTable.insert( {"SCREEN", 0x4000});
 symbolTable.insert( {"KBD", 0x6000});
 
 int32_t currentLocAtTable = 16;
 // first pass, get all those reference (in brackets)
 if (asmFile.is_open())
 {
  while (std::getline(asmFile, currentLine))
  {
   // clean the line, would return empty string if the entire string is a comment
   currentLine = SanitizeIns(currentLine);
 
   // get label
   if (currentLine[0] == '(')
   {
    // remove the braces '(' ')'
    currentLine.erase(std::remove_if(currentLine.begin(), currentLine.end(), [](char c) {return (c == '(' || c == ')'); }), currentLine.end());
 
    symbolTable.insert({ currentLine, instructionList.size() });
   }
 
   // insert sanitized instruction
   else if (!currentLine.empty())
   {
    instructionList.push_back(currentLine);
   }
 
   // ignore if empty or whitespace
  }
 
  std::string buffer;
 
  // second pass
  for (size_t i = 0; i < instructionList.size(); i++)
  {
   currentLine = instructionList[i];
 
   // a-type instruction
   if (currentLine[0] == '@')
   {
    // if the address uses label, replace it
    if (!isdigit(currentLine[1]))
    {
     // strip the @ from the string
     buffer = currentLine.substr(1);
 
     if (symbolTable.find(buffer) == symbolTable.end())
     {
      symbolTable.insert({ buffer, currentLocAtTable++ });
     }
     std::stringstream ss;
     ss << "@" << symbolTable[buffer];
     instructionList[i] = ss.str();
    }
   }
  }
 }
 
 else
 {
  std::cout << "File not found" << "\n";
  return;
 }
 asmFile.close();
 
 std::cout << "\n" << "Symbol table" << "\n";
 for (const auto& it : symbolTable)
 {
  std::cout << it.first << " " << it.second << "\n";
 }
 
 for (int i = 0; i < instructionList.size(); i++)
 {
  std::cout << i << " : \t" << instructionList[i] << "\n";
 }
}
 
// AssembleInstruction
// Input :
//  a list of cleaned instructions (vector)
// Process :
//  check what command is it
//  for a-command
//   if first char is @, then it's a-command
//    to convert to binary
//     convert the value to binary
//  
//  for c-instruction
//   split to 3 parts
//   remember the format of the instruction
//   dest = comp ; jump
//   1 1 1 a c1 c2 c4 c5 c5 c6 d1 d2 d3 j1 j2 j3
//      <------comp-------&gt; <--des-> <-jump->
//   either the dest or jump field may be empty
//    if dest is empty, no =
//    if jump is empty, no ;
//   for comp
//    have both, so comp is somewhere in between dest and jump
//     dest = comp ; jump
//    dest, comp would be after =
//     dest = comp
//    jump, comp would be before ;
//     comp ; jump
//    only comp
//     comp
//   identify comp
//    no easy way to get around this, so
//    compare each comp to the table, and put the equivalent binary in the list
 
/*
-----------------------------------------
 | comp  | c1 c2 c3 c4 c5 c6 |
 ----------------------------------------
 | 0 |  | 1  0  1  0  1  0 |
 | 1 |  | 1  1  1  1  1  1 |
 | -1 |  |   1  1  1  0  1  0 |
 | D |  |   0  0  1  1  0  0 |
 | A | M |   1  1  0  0  0  0 |
 | !D |  |   0  0  1  1  0  1 |
 | !A | !M | 1  1  0  0  0  1 |
 | -D |  | 0  0  1  1  1  1 |
 | -A |  | 1  1  0  0  1  1 |
 | D+1 |  | 0  1  1  1  1  1 |
 | A+1 | M+1 | 1  1  0  1  1  1 |
 | D-1 |  | 0  0  1  1  1  0 |
 | A-1 | M-1 | 1  1  0  0  1  0 |
 | D+A | D+M | 0  0  0  0  1  0 |
 | D-A | D-M | 0  1  0  0  1  1 |
 | A-D | M-D | 0  0  0  1  1  1 |
 | D&A | D&M | 0  0  0  0  0  0 |
 | D|A | D|M | 0  1  0  1  0  1 |
-----------------------------------------
 | a=0 | a=1 |      |
 ----------------------------------------
 */
 // CTRL + K, CTRL + D for auto indent (hold CTRL)
std::vector<std::string> AssembleInstruction(const std::vector<std::string>& instructionList)
{
 std::vector<std::string> binaryInstructionList;
 
 for (const auto& instruction : instructionList)
 {
  // A-type instruction
  if (instruction[0] == '@')
  {
   std::string buffer = instruction.substr(1);
   uint32_t add = std::stoi(buffer);
 
   std::stringstream ss;
 
   ss << std::bitset<16>(add);
   
   std::cout << ss.str() << "\n";
 
   binaryInstructionList.push_back(ss.str());
  }
 
  // C-type instruction
  // dest = comp ; jump
  // either the dest or jump field may be empty
  // if dest is empty, no =
  // if jump is empty, no ;
  else
  {
   // initialize
   uint16_t jump = 0b0;
   uint16_t dest = 0b0;
   uint16_t comp = 0b0;
 
   if (instruction.find(";") != std::string::npos)
   {
    // get the loc
    size_t loc = instruction.find(";");
 
    // get substr
    std::string buffer = instruction.substr(loc + 1);
	
	// std::cout << "comp, buffer = " << buffer << "\n";
 
    // find operation
    if (buffer == "JGT")
    {
     jump = 0b001;
    }
    else if (buffer == "JEQ")
    {
     jump = 0b010;
    }
    else if (buffer == "JGE")
    {
     jump = 0b011;
    }
    else if (buffer == "JLT")
    {
     jump = 0b100;
    }
    else if (buffer == "JNE")
    {
     jump = 0b101;
    }
    else if (buffer == "JLE")
    {
     jump = 0b110;
    }
    // JMP
    else if (buffer == "JMP")
    {
     jump = 0b111;
    }
    else
    {
     std::cout << "Invalid jump instruction" << "\n";
    }
   }
   else
   {
    jump = 0b000;
   }
 
   // dest
   if (instruction.find("=") != std::string::npos)
   {
    // get loc of =
    size_t loc = instruction.find("=");
 
    // get substr (0 till loc - 1)
    std::string buffer = instruction.substr(0, loc);
 
    // 
 
    // get the right operation
    if (buffer == "M")
    {
     dest = 0b001;
    }
    else if (buffer == "D")
    {
     dest = 0b010;
    }
    else if (buffer == "MD")
    {
     dest = 0b011;
    }
    else if (buffer == "A")
    {
     dest = 0b100;
    }
    else if (buffer == "AM")
    {
     dest = 0b101;
    }
    else if (buffer == "AD")
    {
     dest = 0b110;
    }
    else if (buffer == "AMD")
    {
     dest = 0b111;
    }
    else
    {
     std::cout << "Invalid dest instruction" << "\n";
    }
   }
 
   // comp
   {
    std::string buffer;
 
    // have both, so comp is somewhere in between dest and jump
    // dest = comp ; jump
    if (instruction.find("=") != std::string::npos && instruction.find(";") != std::string::npos)
    {
     size_t loc_start = instruction.find("=");
     size_t loc_end = instruction.find(";");
 
     size_t size_buf = loc_end - loc_start + 1;
 
     buffer = instruction.substr(loc_start, size_buf);
    }
 
    // comp is to the right of dest (no jump here)
    // dest = comp
    else if (instruction.find("=") != std::string::npos)
    {
     size_t loc_start = instruction.find("=");
 
     buffer = instruction.substr(loc_start + 1);
    }
 
    // comp is to the left of jump (no dest)
    // i.e comp ; jump
    else if (instruction.find(";") != std::string::npos)
    {
     size_t loc_end = instruction.find(";");
 
     buffer = instruction.substr(0, loc_end);
    }
 
    else
    {
     buffer = instruction;
    }
 
    // Determine instruction
    if (buffer == "0")
    {
     comp = 0b101010;
    }
    else if (buffer == "1")
    {
     comp = 0b111111;
    }
    else if (buffer == "-1")
    {
     comp = 0b111010;
    }
    else if (buffer == "D")
    {
     comp = 0b001100;
    }
    else if (buffer == "A")
    {
     comp = 0b110000;
    }
    else if (buffer == "!D")
    {
     comp = 0b001101;
    }
    else if (buffer == "!A")
    {
     comp = 0b110001;
    }
    else if (buffer == "-D")
    {
     comp = 0b001111;
    }
    else if (buffer == "-A")
    {
     comp = 0b110011;
    }
    else if (buffer == "D+1")
    {
     comp = 0b011111;
    }
    else if (buffer == "A+1")
    {
     comp = 0b110111;
    }
    else if (buffer == "D-1")
    {
     comp = 0b001110;
    }
    else if (buffer == "A-1")
    {
     comp = 0b110010;
    }
    else if (buffer == "D+A")
    {
     comp = 0b000010;
    }
    else if (buffer == "D-A")
    {
     comp = 0b010011;
    }
    else if (buffer == "A-D")
    {
     comp = 0b000111;
    }
    else if (buffer == "D&A")
    {
     comp = 0b0;
    }
    else if (buffer == "D|A")
    {
     comp = 0b010101;
    }
    // double checking is hell
    else if (buffer == "M")
    {
     comp = 0b1110000;
    }
    else if (buffer == "!M")
    {
     comp = 0b1110001;
    }
    else if (buffer == "-M")
    {
     comp = 0b1110011;
    }
    else if (buffer == "M+1")
    {
     comp = 0b1110111;
    }
    else if (buffer == "M-1")
    {
     comp = 0b1110010;
    }
    else if (buffer == "D+M")
    {
     comp = 0b1000010;
    }
    else if (buffer == "D-M")
    {
     comp = 0b1010011;
    }
    else if (buffer == "M-D")
    {
     comp = 0b1000111;
    }
    else if (buffer == "D&M")
    {
     comp = 0b1000000;
    }
    else if (buffer == "D|M")
    {
     comp = 0b1010101;
    }
    else
    {
     std::cout << "Unhandled comp instruction" << "\n";
     std::cout << "buffer = " << buffer << "\n";
    }
   }
 
   // Combine
   dest = dest << 3;
   comp = comp << 6;
 
   uint16_t paddingTop = 0xE000; // 0b1110 0000 0000 0000
   uint16_t binaryIns = 0x0;
 
	// fixed 4.23 AM 22-11, mixing jump
   binaryIns = dest | comp | jump | paddingTop;
 
   // std::cout << "binary = " << std::bitset<16>(binaryIns)<< "\n";
 
   binaryInstructionList.push_back(std::bitset<16>(binaryIns).to_string());
  }
 
 
 }
 
 for (auto ins : binaryInstructionList)
 {
  // std::cout << ins << "\n";
 }
 
 return binaryInstructionList;
}
 
void saveToFile(std::string fileLoc, std::vector<std::string> binaryInstructionList)
{
 std::ofstream outFile;
 outFile.open(fileLoc);
 
 if (outFile.is_open())
 {
  for (auto ins : binaryInstructionList)
  {
   outFile << ins << "\n";
  }
 }
 else
 {
  std::cout << "Error : Fail to open file to write" << "\n";
 }
 
}
 
int main(int argc, char* argv[])
{
 std::vector<std::string> instructionList;
 std::string writeFileLocation = "Pong.hack";
 std::string inputFile = "Pong.asm";
 
 LoadInstructionFromFile(instructionList, inputFile);
 
 
 
 saveToFile(writeFileLocation, AssembleInstruction(instructionList));
 return 0;
}