#ifndef SB_H
#define SB_H
#include "R.h"
#include "I.h"
#include <bits/stdc++.h>
#include <string>
#include <unordered_map>
#include "utils.h" // Include utilities header file
using namespace std;
extern unordered_map<int, int> registers;


// FUNC3 for SB-type instructions
string fu3(string Mnemonic)
{
    if (Mnemonic == "beq")
        return "000";
    else if (Mnemonic == "bne")
        return "001";
    else if (Mnemonic == "blt")
        return "100";
    else if (Mnemonic == "bge" || Mnemonic == "ble")
        return "101";  // Use same funct3 as bge
    else {
        printf("Error: Invalid SB-format instruction\n");
        exit(0);
    }
}


// SB-format instruction processing with binary formatting
pair<string, string> SB(string s, int j, string Mnemonic, unordered_map<string, int> m_label, int PC)
{
    string rs1 = "", rs2 = "", label = "";
    int flag = 0;
    string ans = "";

    for (int i = j; i < s.size(); i++)
    {
        if (s[i] == 'x')
        {
            i++;
            if (flag == 0)
            {
                while (s[i] != ',' && s[i] != ' ')
                {
                    rs1 += s[i];
                    i++;
                }
                flag++;
            }
            else if (flag == 1)
            {
                while ((s[i] != ',' && s[i] != ' ') || rs2 == "")
                {
                    rs2 += s[i];
                    i++;
                }
                flag++;
            }
        }
        if (flag == 2) {
            // Find the start of the label (skip any spaces or commas)
            while (i < s.size() && (s[i] == ' ' || s[i] == ',')) i++;
        
            // Capture the rest of the line as label
            while (i < s.size() && s[i] != ' ' && s[i] != '#') {
                label += s[i];
                i++;
            }
        
            break; // Label found
        }
        
    }

    // Check if label exists
    if (m_label.find(label) == m_label.end())
    {
        printf("Error: Undefined label %s\n", label.c_str());
        exit(0);
    }

    // Get label address and calculate offset
    int pc_2 = m_label[label];
    int offset = pc_2 - PC;

    // Opcode for SB-format
    string opcode = "1100011";

    // Convert offset to 13-bit binary (Two’s Complement)
    string imm = dtb(offset, 13);

    // Extract immediate bits
    string imm_12 = imm.substr(0, 1);   // imm[12]
    string imm_10_5 = imm.substr(1, 6); // imm[10:5] (Fix: Extract 6 bits)
    string imm_4_1 = imm.substr(7, 4);  // imm[4:1] (Fix: Extract 4 bits)
    string imm_11 = imm.substr(11, 1);  // imm[11]

    // Get funct3
    string funct3 = fu3(Mnemonic);

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
    ans = imm_12 + imm_10_5 + rs2_bin + rs1_bin + funct3 + imm_4_1 + imm_11 + opcode;

    // Convert binary to hexadecimal
    string hex = bintodec(ans);

    // Format binary instruction with hyphens
    string formatted_binary = opcode + "-" + funct3 + "-NULL-NULL-" + rs1_bin + "-" + rs2_bin + "-" + imm_12 + imm_11 + imm_10_5 + imm_4_1;

    return {hex, formatted_binary};
}

#endif
tuple<string, int, int, int> decode_SB(const string& line, unordered_map<string, int>& m_label, int currentPC) {
    stringstream ss(line);
    string mnemonic, rs1_str, rs2_str, label;

    ss >> mnemonic >> rs1_str >> rs2_str >> label;

    if (rs1_str.back() == ',') rs1_str.pop_back();
    if (rs2_str.back() == ',') rs2_str.pop_back();

    int rs1 = stoi(rs1_str.substr(1));
    int rs2 = stoi(rs2_str.substr(1));

    if (m_label.find(label) == m_label.end()) {
        cerr << "Decode Error: Label \"" << label << "\" not found.\n";
        exit(1);
    }

    int offset = m_label[label] - currentPC;

    cout << "Decode Stage: Decoded SB-type instruction.\n";
    cout << "   Mnemonic: " << mnemonic << ", rs1: x" << rs1 << ", rs2: x" << rs2 << ", target offset: " << offset << "\n";

    return {mnemonic, rs1, rs2, offset};
}
bool execute_SB(const string& mnemonic, int rs1_val, int rs2_val) {
    bool result;
    cout << "Execute Stage: ";

    if (mnemonic == "beq") {
        result = (rs1_val == rs2_val);
        cout << "Checking if " << rs1_val << " == " << rs2_val << " → " << (result ? "true" : "false") << "\n";
    }
    else if (mnemonic == "bne") {
        result = (rs1_val != rs2_val);
        cout << "Checking if " << rs1_val << " != " << rs2_val << " → " << (result ? "true" : "false") << "\n";
    }
    else if (mnemonic == "blt") {
        result = (rs1_val < rs2_val);
        cout << "Checking if " << rs1_val << " < " << rs2_val << " → " << (result ? "true" : "false") << "\n";
    }
    else if (mnemonic == "bge") {
        result = (rs1_val >= rs2_val);
        cout << "Checking if " << rs1_val << " >= " << rs2_val << " → " << (result ? "true" : "false") << "\n";
    }
    else if (mnemonic == "ble") {
        result = (rs1_val <= rs2_val);
        cout << "Checking if " << rs1_val << " <= " << rs2_val << " → " << (result ? "true" : "false") << "\n";
    }
    else {
        cerr << "Execute Error: Unknown SB-type mnemonic \"" << mnemonic << "\"\n";
        exit(1);
    }

    return result;
}



void memoryAccessSB(const string& mnemonic) {
    cout << "Memory Access Stage: No memory access for branch instruction \"" << mnemonic << "\"\n";
}
void writeBackSB(const string& mnemonic, bool branchTaken, int targetAddr) {
    if (branchTaken) {
        cout << "Writeback Stage: Branch taken → PC set to " << targetAddr << " (due to \"" << mnemonic << "\")\n";
        cout << "   Result: Branch Taken \n";
    } else {
        cout << "Writeback Stage: Branch not taken → Proceeding sequentially\n";
        cout << "   Result: Branch Not Taken \n";
    }
}
