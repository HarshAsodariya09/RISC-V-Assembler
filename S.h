#ifndef S_H
#define S_H
#include <bits/stdc++.h>
#include <string>
#include "I.h"
#include "R.h"
#include "utils.h" // Include utilities header file
extern unordered_map<int, int> registers;

using namespace std;

// FUNC3 for S-type instructions
string funn3(string Mnemonic)
{
    if (Mnemonic == "sd")
        return "011";
    else if (Mnemonic == "sw")
        return "010";
    else if (Mnemonic == "sh")
        return "001";
    else if (Mnemonic == "sb")
        return "000"; // Fix: Explicitly include "sb" case
    else
    {
        printf("Error: Invalid S-format instruction\n");
        exit(0);
    }
}

// S-format instruction processing with binary formatting
pair<string, string> S(string s, int j, string Mnemonic)
{
    string rs1 = "", rs2 = "", imm = "";
    int flag = 0;
    string ans = "";

    // Extract `rs1`, `rs2`, `imm` fields
    for (int i = j; i < s.size(); i++)
    {
        if (s[i] == 'x' && flag == 0)
        {
            i++;
            while (s[i] != ',' && s[i] != ' ')
            {
                rs2 += s[i];
                i++;
            }
            flag++;
        }
        else if (flag == 1 && s[i] != ' ')
        {
            while (s[i] != '(')
            {
                imm += s[i];
                i++;
            }
            flag++;
        }
        else if (flag == 2 && s[i] != ' ')
        {
            if (s[i] == 'x')
            {
                i++;
                while (s[i] != ')' && s[i] != ' ')
                {
                    rs1 += s[i];
                    i++;
                }
                break;
            }
        }
    }

    // Opcode for S-format
    string opcode = "0100011";

    // Convert immediate to 12-bit binary
    int imm_num = stoi(imm);
    string imm_complete = dectobin(imm_num, 12);

    // Split immediate field
    string imm_high = imm_complete.substr(0, 7); // imm[11:5]
    string imm_low = imm_complete.substr(7, 5);  // imm[4:0]

    // Get funct3
    string funct3 = funn3(Mnemonic);

    // Convert rs1 and rs2 to 5-bit binary
    int rs1_num = stoi(rs1);
    int rs2_num = stoi(rs2);

    if (rs1_num > 31 || rs1_num < 0 || rs2_num > 31 || rs2_num < 0)
    {
        printf("Error: Register not found\n");
        exit(0);
    }

    string rs1_bin = dectobin(rs1_num, 5);
    string rs2_bin = dectobin(rs2_num, 5);

    // Construct binary instruction
    ans = imm_high + rs2_bin + rs1_bin + funct3 + imm_low + opcode;

    // Convert binary to hexadecimal
    string hex = bintodec(ans);

    // Format binary instruction with hyphens
    string formatted_binary =  opcode + "-" + funct3 + "-NULL-" + "NULL-" + rs1_bin + "-" + rs2_bin + "-" + imm_low + "-" + imm_high;


    return {hex, formatted_binary};
}

#endif
tuple<string, int, int, int> decode_S(const string& line) {
    stringstream ss(line);
    string mnemonic, rs2_str, offset_rs1;
    ss >> mnemonic >> rs2_str >> offset_rs1;

    // Clean trailing commas
    if (rs2_str.back() == ',') rs2_str.pop_back();

    // Extract imm(rs1)
    size_t open = offset_rs1.find('(');
    size_t close = offset_rs1.find(')');
    string imm_str = offset_rs1.substr(0, open);
    string rs1_str = offset_rs1.substr(open + 1, close - open - 1);

    int rs1 = stoi(rs1_str.substr(1));
    int rs2 = stoi(rs2_str.substr(1));
    int imm = stoi(imm_str);

    cout << "Decode Stage: Decoded S-type instruction.\n";
    cout << "   Mnemonic: " << mnemonic << ", rs2 (Rm): x" << rs2 << ", rs1: x" << rs1 << ", imm: " << imm << "\n";

    return {mnemonic, rs1, rs2, imm};
}
int execute_S(const string& mnemonic, int rs1_val, int imm) {
    int address = rs1_val + imm;
    cout << "Execute Stage: Effective memory address = x" << rs1_val << " + " << imm << " = " << address << "\n";
    return address;
}
void memoryAccessS(const string& mnemonic, int address, int rs2_val) {
    if (mnemonic == "sd") {
        execMemory[address] = rs2_val;
        cout << "Memory Access Stage: Stored DOUBLE = " << rs2_val << " to address 0x" << hex << address << dec << "\n";
    }
    else if (mnemonic == "sw") {
        execMemory[address] = rs2_val & 0xFFFFFFFF; // lower 32 bits
        cout << "Memory Access Stage: Stored WORD = " << (rs2_val & 0xFFFFFFFF) << " to address 0x" << hex << address << dec << "\n";
    }
    else if (mnemonic == "sh") {
        execMemory[address] = rs2_val & 0xFFFF; // lower 16 bits
        cout << "Memory Access Stage: Stored HALF = " << (rs2_val & 0xFFFF) << " to address 0x" << hex << address << dec << "\n";
    }
    else if (mnemonic == "sb") {
        execMemory[address] = rs2_val & 0xFF; // lower 8 bits
        cout << "Memory Access Stage: Stored BYTE = " << (rs2_val & 0xFF) << " to address 0x" << hex << address << dec << "\n";
    }
    else {
        cerr << "Unknown store mnemonic: " << mnemonic << "\n";
        exit(1);
    }
}

void writeBackS(const string& mnemonic) {
    cout << "Writeback Stage: No register writeback for store instruction \"" << mnemonic << "\"\n";
}
