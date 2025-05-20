#ifndef U_H
#define U_H
#include <bits/stdc++.h>
#include <string>
#include "R.h"
#include "I.h"
#include "utils.h" // Include utilities header file
using namespace std;
extern unordered_map<int, int> registers;


// U-format instruction processing with binary formatting
pair<string, string> U(string s, int j, string Mnemonic)
{
    string ans = "", rd = "", imm = "";
    int flag = 0;

    for (int i = j; i < s.size(); i++)
    {
        if (s[i] == 'x' && flag == 0)
        {
            i++;
            while (s[i] != ',' && s[i] != ' ')
            {
                rd += s[i];
                i++;
            }
            flag++;
        }
        else if (flag == 1)
        {
            while (s[i] == ' ' || s[i] == ',') i++;
            while (i < s.size() && s[i] != ' ')
            {
                imm += s[i];
                i++;
            }
            break;
        }
    }

    string opcode;
    if (Mnemonic == "auipc")
        opcode = "0010111";
    else if (Mnemonic == "lui")
        opcode = "0110111";
    else {
        cerr << "Error: Invalid U-format instruction: " << Mnemonic << "\n";
        exit(1);
    }

    int rd_num = stoi(rd);
    string rd_bin = dectobin(rd_num, 5);

    long long imm_num = stoll(imm);
    string imm_bin = dectobin(imm_num, 20);

    string binary = imm_bin + rd_bin + opcode;
    string hex = bintodec(binary);

    string formatted = opcode + "-NULL-NULL-" + rd_bin + "-NULL-NULL-" + imm_bin;
    return {hex, formatted};
}


#endif
tuple<string, int, int> decode_U(const string& line) {
    stringstream ss(line);
    string mnemonic, rd_str, imm_str;

    ss >> mnemonic >> rd_str >> imm_str;
    if (!rd_str.empty() && rd_str.back() == ',') rd_str.pop_back();

    int rd = stoi(rd_str.substr(1));
    long long imm;
if (imm_str.find("0x") == 0 || imm_str.find("0X") == 0)
    imm = stoll(imm_str, nullptr, 16);  // hex base
else
    imm = stoll(imm_str);  // decimal

    cout << "Decode Stage: Decoded U-type instruction.\n";
    cout << "   Mnemonic: " << mnemonic << ", rd: x" << rd << ", imm (upper 20 bits): " << imm << "\n";

    return {mnemonic, rd, imm};
}


int execute_U(const string& mnemonic, int imm, int PC) {
    if (mnemonic == "lui") {
        int val = imm << 12;
        cout << "Execute Stage: Loading upper immediate = " << val << "\n";
        return val;
    } else if (mnemonic == "auipc") {
        int val = PC + (imm << 12);
        cout << "Execute Stage: auipc result = PC + (imm << 12) = " << PC << " + " << (imm << 12) << " = " << val << "\n";
        return val;
    } else {
        cerr << "Execute Error: Unknown U-type mnemonic " << mnemonic << "\n";
        exit(1);
    }
}

void memoryAccessU(const string& mnemonic) {
    cout << "Memory Access Stage: No memory operation for \"" << mnemonic << "\" instruction.\n";
}
void writeBackU(const string& mnemonic, int rd, int value) {
    if (rd != 0) {
        registers[rd] = value;
        cout << "Writeback Stage: x" << rd << " = " << value << " (Result of " << mnemonic << ")\n";
    } else {
        cout << "Writeback Stage: Skipped writing to x0 (always 0)\n";
    }
}
