#ifndef R_H
#define R_H
#include <bits/stdc++.h>
#include <string>
#include "utils.h" // Include utilities header file
extern unordered_map<int, int> registers;

using namespace std;

// FUNC3 for instruction
string func3(string Mnemonic)
{
    if (Mnemonic == "add" || Mnemonic == "sub" || Mnemonic == "mul")
        return "000";
    else if (Mnemonic == "sll")
        return "001";
    else if (Mnemonic == "slt")
        return "010";
    else if (Mnemonic == "xor" || Mnemonic == "div")
        return "100";
    else if (Mnemonic == "srl" || Mnemonic == "sra")
        return "101";
    else if (Mnemonic == "or" || Mnemonic == "rem")
        return "110";
    else
        return "111";
}

// FUNC7 for instruction
string func7(string Mnemonic)
{
    if (Mnemonic == "sub" || Mnemonic == "sra")
        return "0100000";
    else if (Mnemonic == "mul" || Mnemonic == "div" || Mnemonic == "rem")
        return "0000001";
    else
        return "0000000";
}

// R-format instruction processing with binary formatting
// R-format instruction processing with binary formatting
pair<string, string> R(string s, int j, string Mnemonic)
{
    string rd = "", rs1 = "", rs2 = "";
    int flag = 0;
    string ans = "";

    // Extract `rd`, `rs1`, `rs2`
    for (int i = j; i < s.size(); i++)
    {
        if (s[i] == 'x')
        {
            i++;
            if (flag == 0)
            {
                while (s[i] != ',' && s[i] != ' ')
                {
                    rd += s[i];
                    i++;
                }
                flag++;
            }
            else if (flag == 1)
            {
                while ((s[i] != ',' && s[i] != ' ') || rs1 == "")
                {
                    rs1 += s[i];
                    i++;
                }
                flag++;
            }
            else if (flag == 2)
            {
                while (i != s.size() && s[i] != ' ')
                {
                    rs2 += s[i];
                    i++;
                }
                flag = 0;
                break;
            }
        }
    }

    // Opcode for R-type
    string opcode = "0110011";

    // ✅ Use base 0 to support both decimal and hex
    int rd_num = stoi(rd, nullptr, 0);
    int rs1_num = stoi(rs1, nullptr, 0);
    int rs2_num = stoi(rs2, nullptr, 0);

    if (rd_num > 31 || rd_num < 0 || rs1_num > 31 || rs1_num < 0 || rs2_num > 31 || rs2_num < 0)
    {
        printf("Error: Register not found\n");
        exit(0);
    }

    string rd_bin = dectobin(rd_num, 5);
    string rs1_bin = dectobin(rs1_num, 5);
    string rs2_bin = dectobin(rs2_num, 5);

    string funct3 = func3(Mnemonic);
    string funct7 = func7(Mnemonic);

    ans = funct7 + rs2_bin + rs1_bin + funct3 + rd_bin + opcode;
    string hex = bintodec(ans);

    string formatted_binary = opcode + "-" + funct3 + "-" + funct7 + "-" + rd_bin + "-" + rs1_bin + "-" + rs2_bin + "-NULL";

    return {hex, formatted_binary};
}

// Decode R-type instruction: extracts mnemonic, rd, rs1, rs2 from the line
tuple<string, int, int, int> decode_R(const string& instructionLine)
{
    stringstream ss(instructionLine);
    string mnemonic, rd_str, rs1_str, rs2_str;

    ss >> mnemonic >> rd_str >> rs1_str >> rs2_str;

    // Remove trailing commas
    if (!rd_str.empty() && rd_str.back() == ',') rd_str.pop_back();
    if (!rs1_str.empty() && rs1_str.back() == ',') rs1_str.pop_back();

    int rd = stoi(rd_str.substr(1));   // x5 → 5
       // base 0 → supports both 5 and 0x5
int rs1 = stoi(rs1_str.substr(1), nullptr, 0);
int rs2 = stoi(rs2_str.substr(1), nullptr, 0);

    return make_tuple(mnemonic, rd, rs1, rs2);
}

int execute_R(const string& mnemonic, int rs1_val, int rs2_val)
{
    cout << "Execute Stage: Performing " << mnemonic << " on values "
         << rs1_val << " and " << rs2_val << ".\n";

    if (mnemonic == "add") return rs1_val + rs2_val;
    if (mnemonic == "sub") return rs1_val - rs2_val;
    if (mnemonic == "and") return rs1_val & rs2_val;
    if (mnemonic == "or")  return rs1_val | rs2_val;
    if (mnemonic == "xor") return rs1_val ^ rs2_val;
    if (mnemonic == "sll") return rs1_val << (rs2_val & 0x1F);
    if (mnemonic == "srl") return (unsigned int)rs1_val >> (rs2_val & 0x1F);
    if (mnemonic == "sra") return rs1_val >> (rs2_val & 0x1F);
    if (mnemonic == "slt") return rs1_val < rs2_val ? 1 : 0;
    if (mnemonic == "mul") return rs1_val * rs2_val;
    if (mnemonic == "div") return rs2_val != 0 ? rs1_val / rs2_val : 0;
    if (mnemonic == "rem") return rs2_val != 0 ? rs1_val % rs2_val : 0;

    cerr << "Error: Unknown R-type mnemonic: " << mnemonic << endl;
    exit(1);
}

// Memory Access stage for R-type (no actual memory operation)
void memoryAccessR(const string& mnemonic) {
    cout << "Memory Access Stage: No memory access required for \"" << mnemonic << "\" instruction.\n";
}



// Write Back stage for R-type instructions (updates destination register)
void writeBackR(int rd, int result, const string& mnemonic)
{
    registers[rd] = result;
    cout << "Writeback Stage: x" << rd << " = " << result << " (Result of " << mnemonic << ")\n";
}


#endif
