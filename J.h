#ifndef J_H
#define J_H

#include <bits/stdc++.h>
#include <string>
#include "utils.h"  // Ensure dtb() and bintodec() functions exist
#include <unordered_map>
using namespace std;
extern unordered_map<int, int> registers;

// J-format instruction processing with binary formatting
pair<string, string> J(string s, int j, string Mnemonic, unordered_map<string, int> m_label, int currentPC)
{
    string ra = "", label = "";
    int flag = 0;
    string ans = "";

    // Extracting rd and label
    for (int i = j; i < s.size(); i++) {
        if (s[i] == 'x' && flag == 0) {
            i++;
            while (s[i] != ',' && s[i] != ' ') {
                ra += s[i];
                i++;
            }
            flag++;
        }
        if (flag == 1) {
            i++;
            while (s[i] == ' ') i++;
            while (i < s.size() && s[i] != ' ') {
                label += s[i];
                i++;
            }
            flag = 0;
            break;
        }
    }

    string opcode = "1101111";
    int ra_num = stoi(ra);
    if (ra_num > 31 || ra_num < 0) {
        cout << "Error: Register not found" << endl;
        exit(0);
    }
    string ra_bin = dtb(ra_num, 5);

    // Correct offset using currentPC
    if (m_label.find(label) == m_label.end()) {
        cout << "Error: Undefined label " << label << endl;
        exit(0);
    }
    int label_addr = m_label[label];
    int offset = label_addr - currentPC;  // ✅ CORRECT

    string imm = dtb(offset, 21);  // signed offset
    string imm_20 = imm.substr(0, 1);
    string imm_10_1 = imm.substr(11, 10);
    string imm_11 = imm.substr(10, 1);
    string imm_19_12 = imm.substr(12, 8);  // ✅ FIXED

    ans = imm_20 + imm_19_12 + imm_11 + imm_10_1 + ra_bin + opcode;
    string hex = bintodec(ans);

    return {hex, ans};
}


tuple<string, int, int> decode_J(const string& line, unordered_map<string, int>& m_label, int currentPC) {
    stringstream ss(line);
    string mnemonic, rd_str, label_or_imm;

    ss >> mnemonic >> rd_str >> label_or_imm;

    if (!rd_str.empty() && rd_str.back() == ',') rd_str.pop_back();

    int rd = stoi(rd_str.substr(1));
    int offset;

    try {
        offset = stoi(label_or_imm);  // Try if numeric
    } catch (...) {
        // Try resolving label
        if (m_label.find(label_or_imm) != m_label.end()) {
            offset = m_label[label_or_imm] - currentPC;
        } else {
            cerr << "Error: Unknown label or invalid offset: " << label_or_imm << "\n";
            exit(1);
        }
    }

    return {mnemonic, rd, offset};
}


#endif
int execute_J(int currentPC, int offset) {
    int target = currentPC + offset;
    cout << "Execute Stage: Calculated jump target address = " << target << "\n";
    return target;
}
void memoryAccessJ(const string& mnemonic) {
    cout << "Memory Access Stage: No memory operation for \"" << mnemonic << "\" instruction.\n";
}
void writeBackJ(int rd, int returnAddress, const string& mnemonic) {
    registers[rd] = returnAddress;
    cout << "Writeback Stage: Saving return address (PC + 4) = " << returnAddress << " to Ry (x" << rd << ") for " << mnemonic << "\n";
}
