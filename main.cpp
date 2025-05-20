
#include <bits/stdc++.h>
#include <string>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include "R.h"
#include "I.h"
#include "S.h"
#include "U.h"
#include "SB.h"
#include "J.h"
#include <utility>
#include <iomanip>

#include <unordered_map>
unordered_map<string, int> data_label_address; // label -> memory address
using namespace std;
unordered_map<string, int> m_label;  // label -> instruction address
unordered_map<int, bool> branch_prediction_table;  // 1-bit branch prediction table (PC -> prediction)
unordered_map<int, int> branch_target_address;    // Store the target address of each branch instruction
int PC = 0;
int PC_2 = 0; // for label
unsigned int dat = 268435456;
int factor = 0;
int clock_cycles = 0; // Global clock variable
unordered_map<int, int> registers;
void handleControlHazard(int& cycle, int& PC, bool branch_taken, int branch_PC, int target_PC);
bool Knob1 = true; // Enable/disable pipelining
bool Knob2 = true; // Enable/disable data forwarding
bool Knob3 = true; // Enable/disable printing all register file content at each cycle
bool Knob4 = false; // Enable/disable printing pipeline registers at the end of each cycle
int Knob5 = 5; // Specific instruction number to track in pipeline registers
bool Knob6 = true; // Enable/disable printing branch prediction details
int data_hazard_stalls = 0;  // Stat11: Number of stalls due to data hazards
int control_hazard_stalls = 0;  // Stat12: Number of stalls due to control hazards
bool flush_next_fetch = false; // New global flag
bool data_hazard_stall_now = false;
bool control_hazard_stall_now = false;
unordered_map<int, int> instruction_fetch_cycle; // instruction_number -> fetch cycle
int instruction_counter = 0;
bool fetched_printed = false;
bool decoded_printed = false;
bool executed_printed = false;
bool memaccess_printed = false;
bool writeback_printed = false;


int instruction_count = 0;
int data_transfer_count = 0;
int alu_count = 0;
int control_count = 0;
int total_stalls = 0;
int data_hazard_count = 0;
int control_hazard_count = 0;
int branch_mispredicts = 0;
// Function to check RAW (Read-After-Write) hazard and return the number of stalls
 // Stat11: Number of stalls due to data hazards
 struct IF_ID_Register {
    bool valid = false;
    string instruction;
    int PC = 0;
} IF_ID;

struct ID_EX_Register {
    bool valid = false;
    string mnemonic = "";
    string type = "";
    int rs1 = -1, rs2 = -1, rd = -1;
    int val1 = 0, val2 = 0, imm = 0;
    int PC = 0;
} ID_EX;

struct EX_MEM_Register {
    bool valid = false;
    string type = "";
    string mnemonic = ""; // ← Add this line
    int rd = -1;
    int PC = 0; // ← ADD THIS
    int aluResult = 0;
    int val2 = 0;
} EX_MEM;

struct MEM_WB_Register {
    bool valid = false;
    string type = "";
    string mnemonic = ""; // ← add this line
    int rd = -1;
    int result = 0;
} MEM_WB;
int get_rs1(const string& instr) {
    istringstream iss(instr);
    string mnemonic, rd, rs1;
    iss >> mnemonic >> rd >> rs1;
    return stoi(rs1.substr(1));
}

int get_rs2(const string& instr) {
    istringstream iss(instr);
    string mnemonic, rd, rs1, rs2;
    iss >> mnemonic >> rd >> rs1 >> rs2;
    return (rs2[0] == 'x') ? stoi(rs2.substr(1)) : -1;  // handle I-type
}

void printRegisterState() {
    cout << "[Register State After This Cycle]\n";

    for (int i = 0; i < 32; i++) {
        if (i == 2)
            cout << left << setw(8) << "sp(x02)" << "= " << setw(10) << registers[i];
        else
            cout << left << setw(8) << ("x" + to_string(i)) << "= " << setw(10) << registers[i];

        if ((i + 1) % 4 == 0) cout << "\n"; // Print 4 registers per line
    }
    cout << "----------------------------------------\n";
}

string hexa_convert(int y) {
    stringstream ss;
    ss << hex << uppercase << setw(8) << setfill('0') << (y & 0xFFFFFFFF); // Ensure 8-digit hex format
    return ss.str();
}

// --------------------------------------------------
// Fetch stage: only grab a new instruction if
// there’s no Instr in IF_ID (i.e. previous fetch
// has moved on or stalled) — don’t check ID_EX here!
// --------------------------------------------------
void instructionFetch(vector<string>& program, int& PC) {
    if (flush_next_fetch) {
        flush_next_fetch = false; // allow next fetch
        IF_ID.valid = false;      // discard anything fetched wrongly
        return;                   // completely skip fetch this cycle
    }
    if (IF_ID.valid) return;

    // Check if current PC is a predicted branch
    if (branch_prediction_table.count(PC) && branch_prediction_table[PC]) {
        if (branch_target_address.count(PC)) {
            IF_ID.valid       = true;
            IF_ID.PC          = PC;
            IF_ID.instruction = program[PC / 4];

            // 🔥 Track this instruction if needed
            if (instruction_counter == Knob5 && !fetched_printed) {
                cout << "[TRACKING] Instruction " << Knob5
                     << " fetched at PC = 0x" << hexa_convert(IF_ID.PC)
                     << " during cycle " << clock_cycles << ".\n";
                fetched_printed = true;  // ✅ Mark printed
            }
            
            instruction_counter++;  // ✅ Increment after fetching real instruction

            PC = branch_target_address[PC];  // Redirect on predicted taken
            return;
        }
    }

    // Normal sequential fetch
    if (PC < (int)program.size() * 4) {
        IF_ID.valid       = true;
        IF_ID.PC          = PC;
        IF_ID.instruction = program[PC / 4];

        // 🔥 Track this instruction if needed
        if (ID_EX.valid && instruction_counter-1 == Knob5 && !decoded_printed) {
            cout << "[TRACKING] Instruction " << Knob5
                 << " decoded during cycle " << clock_cycles << ".\n";
            decoded_printed = true;  // ✅ Mark printed
        }
        
        instruction_counter++;  // ✅ Increment after fetching real instruction

        PC += 4;
    } else {
        IF_ID.valid = false;
    }
}



// R format
vector<string> Rf;
// I format
vector<string> If;
// S format
vector<string> Sf;
// UJ format
vector<string> Uf;
// SB format
vector<string> Sbf;
// J format
vector<string> Jf;
void instructionDecode() {
    if (!IF_ID.valid) return;

    //─── 1) Grab the raw fields from IF/ID ────────────────────────────────
    istringstream iss(IF_ID.instruction);
    string mnemonic;
    iss >> mnemonic;

    // quickly handle an empty line
    if (mnemonic.empty()) {
        IF_ID.valid = false;
        return;
    }

    int rd = -1, rs1 = -1, rs2 = -1;
    string rd_s, rs1_s, rs2_s, imm, label;

    //──── EARLY special handling for jal ────────────────────────────────
    if (mnemonic == "jal") {
        // parse operands
        flush_next_fetch = true;

        iss >> rd_s >> label;
        rd = (rd_s[0]=='x') ? stoi(rd_s.substr(1)) : -1;

        int jump_target = 0;
        if (m_label.count(label)) {
            jump_target = m_label[label];
        } else {
            jump_target = 0; // fallback or error
        }

        // ✅ Write return address into rd
        if (rd != 0 && rd != -1)
            registers[rd] = IF_ID.PC + 4;

        // ✅ Update branch prediction tables (optional)
        branch_prediction_table[IF_ID.PC] = true;
        branch_target_address[IF_ID.PC] = jump_target;

        // ✅ Flush pipeline
        IF_ID.valid = false;
        ID_EX.valid = false;
        EX_MEM.valid = false;
        MEM_WB.valid = false;

        // ✅ Redirect PC
        PC = jump_target;

        // ✅ Update stats
        instruction_count++;
        control_count++;

        return; // exit early, don't fill ID_EX
    }
    //──── Normal decode continues ───────────────────────────────────────

    // parse operands but do not touch ID_EX yet
    if (find(Rf.begin(), Rf.end(), mnemonic) != Rf.end()) {
        iss >> rd_s >> rs1_s >> rs2_s;
        rd  = (rd_s[0]=='x')  ? stoi(rd_s.substr(1))  : -1;
        rs1 = (rs1_s[0]=='x') ? stoi(rs1_s.substr(1)) : -1;
        rs2 = (rs2_s[0]=='x') ? stoi(rs2_s.substr(1)) : -1;
    }
    else if (find(If.begin(), If.end(), mnemonic) != If.end()) {
        iss >> rd_s >> rs1_s >> imm;
        rd  = (rd_s[0]=='x')  ? stoi(rd_s.substr(1))  : -1;
        rs1 = (rs1_s[0]=='x') ? stoi(rs1_s.substr(1)) : -1;
    }
    else if (find(Sf.begin(), Sf.end(), mnemonic) != Sf.end()) {
        iss >> rs2_s >> imm >> rs1_s;
        rs1 = (rs1_s[0]=='x') ? stoi(rs1_s.substr(1)) : -1;
        rs2 = (rs2_s[0]=='x') ? stoi(rs2_s.substr(1)) : -1;
    }
    else if (find(Sbf.begin(), Sbf.end(), mnemonic) != Sbf.end()) {
        iss >> rs1_s >> rs2_s >> label;
        rs1 = (rs1_s[0]=='x') ? stoi(rs1_s.substr(1)) : -1;
        rs2 = (rs2_s[0]=='x') ? stoi(rs2_s.substr(1)) : -1;
    }
    else if (find(Uf.begin(), Uf.end(), mnemonic) != Uf.end()) {
        iss >> rd_s >> imm;
        rd  = (rd_s[0]=='x') ? stoi(rd_s.substr(1)) : -1;
    }
    else if (find(Jf.begin(), Jf.end(), mnemonic) != Jf.end()) {
        iss >> rd_s >> label;
        rd  = (rd_s[0]=='x') ? stoi(rd_s.substr(1)) : -1;
    }

    //─── 2) RAW hazard check (stall if no forwarding) ──────────────────
    if (!Knob2 && EX_MEM.valid && EX_MEM.rd != 0 &&
        (EX_MEM.rd == rs1 || EX_MEM.rd == rs2))
    {
        total_stalls++;
        data_hazard_stalls++;
        data_hazard_count++;
        data_hazard_stall_now = true;  // ✅ Set the stall flag
        // leave IF_ID.valid==true so we retry this decode next cycle
        return;
    }
    

    //─── 3) Now that we’re past the stall check, fill ID/EX ───────────────
    ID_EX.valid    = true;
    ID_EX.mnemonic = mnemonic;
    ID_EX.PC       = IF_ID.PC;
    ID_EX.rd       = rd;
    ID_EX.rs1      = rs1;
    ID_EX.rs2      = rs2;
    if (instruction_counter-1 == Knob5) {
        cout << "[TRACKING] Instruction " << Knob5 
             << " decoded during cycle " << clock_cycles << ".\n";
    }
    
    if (rs1 >= 0) ID_EX.val1 = registers[rs1];
    if (rs2 >= 0) ID_EX.val2 = registers[rs2];

    // set type & immediate
    if (find(Rf.begin(), Rf.end(), mnemonic) != Rf.end()) {
        ID_EX.type = "R";
    }
    else if (find(If.begin(), If.end(), mnemonic) != If.end()) {
        ID_EX.type = "I";
        ID_EX.imm = imm.empty() ? 0 : stoi(imm);
    }
    else if (find(Sf.begin(), Sf.end(), mnemonic) != Sf.end()) {
        ID_EX.type = "S";
        ID_EX.imm = imm.empty() ? 0 : stoi(imm);
    }
    else if (find(Sbf.begin(), Sbf.end(), mnemonic) != Sbf.end()) {
        iss >> rs1_s >> rs2_s >> label;
        rs1 = (rs1_s[0]=='x') ? stoi(rs1_s.substr(1)) : -1;
        rs2 = (rs2_s[0]=='x') ? stoi(rs2_s.substr(1)) : -1;

        ID_EX.imm = m_label.count(label) ? m_label[label] - IF_ID.PC : 0;
    }
    else if (find(Uf.begin(), Uf.end(), mnemonic) != Uf.end()) {
        ID_EX.type = "U";
        ID_EX.imm = imm.empty() ? 0 : stoi(imm);
    }
    else if (find(Jf.begin(), Jf.end(), mnemonic) != Jf.end()) {
        ID_EX.type = "J";
        iss >> rd_s >> label;
        rd  = (rd_s[0]=='x') ? stoi(rd_s.substr(1)) : -1;
        ID_EX.imm = m_label.count(label) ? (m_label[label] - IF_ID.PC) : 0;
    }

    //─── 4) Data‐forwarding (if enabled) ─────────────────────────────────
    if (Knob2) {
        if (EX_MEM.valid && EX_MEM.rd != 0) {
            if (EX_MEM.rd == rs1) ID_EX.val1 = EX_MEM.aluResult;
            if (EX_MEM.rd == rs2) ID_EX.val2 = EX_MEM.aluResult;
        }
        if (MEM_WB.valid && MEM_WB.rd != 0) {
            if (MEM_WB.rd == rs1) ID_EX.val1 = MEM_WB.result;
            if (MEM_WB.rd == rs2) ID_EX.val2 = MEM_WB.result;
        }
    }

    //─── 5) Finally, clear IF/ID so Fetch can advance ────────────────────
    IF_ID.valid = false;
}

void executeStage() {
    if (!ID_EX.valid) return;

    EX_MEM.valid = true;
    EX_MEM.rd = ID_EX.rd;
    EX_MEM.type = ID_EX.type;
    EX_MEM.PC = ID_EX.PC;  // ✅ Preserve PC for later use
    EX_MEM.val2 = ID_EX.imm;       // target offset       // ensure correct branch_PC saved
    // Add this to executeStage():
    EX_MEM.mnemonic = ID_EX.mnemonic;


    if (ID_EX.type == "R") {
        if (ID_EX.mnemonic == "add") {
            EX_MEM.aluResult = ID_EX.val1 + ID_EX.val2;
        } else if (ID_EX.mnemonic == "sub") {
            EX_MEM.aluResult = ID_EX.val1 - ID_EX.val2;
        } else if (ID_EX.mnemonic == "and") {
            EX_MEM.aluResult = ID_EX.val1 & ID_EX.val2;
        } else if (ID_EX.mnemonic == "or") {
            EX_MEM.aluResult = ID_EX.val1 | ID_EX.val2;
        } else if (ID_EX.mnemonic == "xor") {
            EX_MEM.aluResult = ID_EX.val1 ^ ID_EX.val2;
        } else if (ID_EX.mnemonic == "sll") {
            EX_MEM.aluResult = ID_EX.val1 << ID_EX.val2;
        } else if (ID_EX.mnemonic == "srl") {
            EX_MEM.aluResult = (unsigned)ID_EX.val1 >> ID_EX.val2;
        } else if (ID_EX.mnemonic == "sra") {
            EX_MEM.aluResult = ID_EX.val1 >> ID_EX.val2;
        } else if (ID_EX.mnemonic == "slt") {
            EX_MEM.aluResult = ID_EX.val1 < ID_EX.val2 ? 1 : 0;
        } else if (ID_EX.mnemonic == "mul") {
            EX_MEM.aluResult = ID_EX.val1 * ID_EX.val2;
        } else if (ID_EX.mnemonic == "div") {
            EX_MEM.aluResult = ID_EX.val2 == 0 ? 0 : ID_EX.val1 / ID_EX.val2;
        } else if (ID_EX.mnemonic == "rem") {
            EX_MEM.aluResult = ID_EX.val2 == 0 ? 0 : ID_EX.val1 % ID_EX.val2;
        }
    } 
    else if (ID_EX.type == "I") {
        if (ID_EX.mnemonic == "addi") {
            EX_MEM.aluResult = ID_EX.val1 + ID_EX.imm;
        } else if (ID_EX.mnemonic == "andi") {
            EX_MEM.aluResult = ID_EX.val1 & ID_EX.imm;
        } else if (ID_EX.mnemonic == "ori") {
            EX_MEM.aluResult = ID_EX.val1 | ID_EX.imm;
        }
        // Add more I-type operations if needed (e.g., loads later)
    }
    else if (ID_EX.type == "S") {
        if (ID_EX.mnemonic == "sb" || ID_EX.mnemonic == "sw" || ID_EX.mnemonic == "sd" || ID_EX.mnemonic == "sh") {
            EX_MEM.aluResult = ID_EX.val1 + ID_EX.imm;
            EX_MEM.val2 = ID_EX.val2;  // value to store
        }
    }
    else if (ID_EX.type == "SB") {
        if (ID_EX.mnemonic == "beq") {
            EX_MEM.aluResult = (ID_EX.val1 == ID_EX.val2);
        } else if (ID_EX.mnemonic == "bne") {
            EX_MEM.aluResult = (ID_EX.val1 != ID_EX.val2);
        } else if (ID_EX.mnemonic == "blt") {
            EX_MEM.aluResult = (ID_EX.val1 < ID_EX.val2);
        } else if (ID_EX.mnemonic == "bge") {
            EX_MEM.aluResult = (ID_EX.val1 >= ID_EX.val2);
        } else if (ID_EX.mnemonic == "ble") {
            EX_MEM.aluResult = (ID_EX.val1 <= ID_EX.val2);
        }
        EX_MEM.val2 = ID_EX.imm;  // store target offset
    }
    else if (ID_EX.type == "U") {
        if (ID_EX.mnemonic == "lui") {
            EX_MEM.aluResult = ID_EX.imm << 12;
        } else if (ID_EX.mnemonic == "auipc") {
            EX_MEM.aluResult = ID_EX.PC + (ID_EX.imm << 12);
        }
    }
    else if (ID_EX.type == "J") {
        // return address in rd
        EX_MEM.aluResult = ID_EX.PC + 4;
        // compute absolute jump target, not just the offset
        EX_MEM.val2      = ID_EX.PC + ID_EX.imm;
    }

    

    ID_EX.valid = false;
    if (EX_MEM.valid && instruction_counter-1 == Knob5 && !executed_printed) {
        cout << "[TRACKING] Instruction " << Knob5
             << " executed during cycle " << clock_cycles << ".\n";
        executed_printed = true;  // ✅ Mark printed
    }
    
    
}


void writeBackStage() {
    if (!MEM_WB.valid) return;

    // ✅ Skip invalid writeback types
    if (MEM_WB.type == "S" || MEM_WB.type == "SB" || MEM_WB.rd == 0 || MEM_WB.rd == -1) {
        MEM_WB.valid = false;
        return;
    }

    // ✅ Actually update register
    registers[MEM_WB.rd] = MEM_WB.result;
    
    
    

    // ✅ Count stats
    instruction_count++;if (MEM_WB.valid && instruction_counter-1 == Knob5 && !writeback_printed) {
        cout << "[TRACKING] Instruction " << Knob5
             << " writeback completed during cycle " << clock_cycles << ".\n";
        writeback_printed = true;  // ✅ Mark printed
    }
    
    if (MEM_WB.type == "I") {
        if (MEM_WB.mnemonic == "addi" || MEM_WB.mnemonic == "andi" || MEM_WB.mnemonic == "ori")
            alu_count++;
        else
            data_transfer_count++;
    }
    else if (MEM_WB.type == "R" || MEM_WB.type == "U") alu_count++;
    else if (MEM_WB.type == "J") control_count++;

    if (Knob3) {
        cout << "Write Back Stage: Writing " << hex << MEM_WB.result
             << " to x" << dec << MEM_WB.rd << "\n";
    }

    MEM_WB.valid = false;
}


void memoryAccessStage() {
    if (!EX_MEM.valid) return;

    MEM_WB.valid = true;
    MEM_WB.type = EX_MEM.type;
    MEM_WB.rd = EX_MEM.rd;
    MEM_WB.mnemonic = ID_EX.mnemonic;

    if (EX_MEM.type == "I") {
        // Assume loads will be handled here later
        MEM_WB.result = EX_MEM.aluResult;  // If this is an ALU op like addi
    } 
    else if (EX_MEM.type == "S") {
        execMemory[EX_MEM.aluResult] = EX_MEM.val2;
        MEM_WB.valid = false;  // No write-back needed for stores
        return;
    } 
    else if (EX_MEM.type == "SB") {
        bool branchTaken = EX_MEM.aluResult;
        int target_PC = EX_MEM.val2;       // Make sure this is absolute address
        int branch_PC = EX_MEM.PC;
    
        // 🟡 Maintain BTB + prediction
        if (Knob6 || true) {  // Always update BTB (not just when printing)
            branch_prediction_table[branch_PC] = branchTaken;
            branch_target_address[branch_PC] = target_PC;
        }
    
        handleControlHazard(clock_cycles, PC, branchTaken, branch_PC, target_PC);
        MEM_WB.valid = false;
        if (MEM_WB.valid && instruction_counter-1 == Knob5 && !memaccess_printed) {
            cout << "[TRACKING] Instruction " << Knob5
                 << " memory access during cycle " << clock_cycles << ".\n";
            memaccess_printed = true;  // ✅ Mark printed
        }
        
        
        return;
    }
    
    
    
        // U- and R-type write-back
        else if (EX_MEM.type == "U" || EX_MEM.type == "R") {
            MEM_WB.result = EX_MEM.aluResult;
        }
        // unconditional JAL: always update BTB and redirect PC immediately
        else if (EX_MEM.type == "J") {
            MEM_WB.result = EX_MEM.aluResult;
        
            // ✅ Store target in BTB (if not already wrong)
            if (EX_MEM.val2 != EX_MEM.PC) {
                branch_prediction_table[EX_MEM.PC] = true;
                branch_target_address[EX_MEM.PC] = EX_MEM.val2;
            }
        
            // ✅ Flush the pipeline to cancel wrong-path instructions
            IF_ID.valid = false;
            ID_EX.valid = false;
            EX_MEM.valid = false;
            MEM_WB.valid = false;
        
            // ✅ Redirect PC
            PC = EX_MEM.val2;
            return;
        }
        
        
    else {
        // fallback
        MEM_WB.result = EX_MEM.aluResult;
    }


    EX_MEM.valid = false;
}


// Function to simulate checking for RAW hazard between two instructions
bool checkRAWHazard(const string& current_instruction, const string& previous_instruction) {
    // Extract registers from current and previous instruction
    istringstream current_stream(current_instruction);
    string current_opcode, current_rd, current_rs1, current_rs2;
    current_stream >> current_opcode >> current_rd >> current_rs1 >> current_rs2;

    current_rd = current_rd.substr(1); // Remove the 'x' (e.g., "x2" -> "2")
    current_rs1 = current_rs1.substr(1); // Remove the 'x'
    current_rs2 = current_rs2.substr(1); // Remove the 'x'

    // Extract from previous instruction
    istringstream previous_stream(previous_instruction);
    string previous_opcode, previous_rd, previous_rs1, previous_rs2;
    previous_stream >> previous_opcode >> previous_rd >> previous_rs1 >> previous_rs2;

    previous_rd = previous_rd.substr(1); // Remove the 'x'

    // Check if the current instruction reads a register that the previous instruction writes to
    if (current_rs1 == previous_rd || current_rs2 == previous_rd) {
        return true; // Data hazard detected
    }

    return false;
}

void processStalls(vector<string>& instructions){
    vector<string> pipeline;  // Simulate the pipeline (F -> D -> E -> M -> W)
    int stall_cycles = 0;  // Track how many cycles to stall for

    for (size_t i = 0; i < instructions.size(); i++) {
        string current_instruction = instructions[i];

        // Check for RAW hazard between current instruction and previous instruction in the pipeline
        if (Knob1 && i > 0 && checkRAWHazard(current_instruction, instructions[i - 1])) {
            stall_cycles++;
            data_hazard_stalls++;
            cout << "Cycle " << stall_cycles << ": Stall inserted due to data hazard between instruction " << (i - 1) << " and instruction " << i << "\n";
        }
        

        // Add current instruction to the pipeline (move it through stages)
        pipeline.push_back(current_instruction);

        // Output the instruction in the pipeline for this cycle
        cout << "Pipeline after Cycle " << stall_cycles << ": ";
        for (const auto& instr : pipeline) {
            cout << instr << " | ";
        }
        cout << "\n";
    }
}
// Function to prompt the user for enabling/disabling a knob
void promptKnobSetting(string knob_name, bool &knob) {
    string input;
    cout << "Do you want to enable " << knob_name << "? (yes/no): ";
    cin >> input;
    if (input == "yes" || input == "y") {
        knob = true;
        cout << knob_name << " has been enabled.\n";
    } else {
        knob = false;
        cout << knob_name << " has been disabled.\n";
    }
}

// Stat12: Number of stalls due to control hazards
// Function to simulate branch prediction and handle control hazards
bool branchPrediction(int branch_PC) {
    // If the branch prediction is not yet set, default to 'not taken' (0)
    if (branch_prediction_table.find(branch_PC) == branch_prediction_table.end()) {
        branch_prediction_table[branch_PC] = false;  // Initial prediction: not taken
    }
    
    // Return the current prediction (0 for not taken, 1 for taken)
    return branch_prediction_table[branch_PC];
}

// Function to update branch prediction table based on the outcome of the branch
void updateBranchPrediction(int branch_PC, bool branch_taken) {
    // Update the branch prediction table with the outcome of the branch
    branch_prediction_table[branch_PC] = branch_taken;
    cout << "Branch prediction updated: ";
    cout << (branch_taken ? "Taken" : "Not Taken") << " for branch at PC = 0x" << hex << branch_PC << "\n";
}

// Function to handle control hazard (branch misprediction)
void handleControlHazard(int& cycle, int& PC, bool branch_taken, int branch_PC, int target_PC) {
    branch_target_address[branch_PC] = target_PC;

    bool prediction = branchPrediction(branch_PC);

    if (prediction != branch_taken) {
        cout << "Cycle " << dec << cycle << ": Branch misprediction at PC = 0x" << hexa_convert(branch_PC) << "\n";
        cout << "Flushing pipeline and restarting from target PC = 0x" << hexa_convert(target_PC) << "\n";
        control_hazard_stall_now = true; // ✅ Stall flag

        // 🔁 FLUSH all intermediate pipeline registers
        IF_ID.valid = false;
        ID_EX.valid = false;
        EX_MEM.valid = false;
        MEM_WB.valid = false;

        control_hazard_stalls++;
        total_stalls += 2;
        cycle += 2;

        PC = target_PC;

        branch_mispredicts++;
        updateBranchPrediction(branch_PC, branch_taken);
    } else {
        PC = branch_taken ? target_PC : PC + 4;
    }
}




// Function to process instructions and handle control hazards
void processControlHazards(vector<string>& instructions){
    for (size_t i = 0; i < instructions.size(); i++) {
        string instruction = instructions[i];

        // If instruction is a branch, handle control hazard
        if (instruction.find("beq") != string::npos || instruction.find("bne") != string::npos || 
            instruction.find("jalr") != string::npos || instruction.find("jal") != string::npos) {

            // Extract the branch address and target address
            int branch_PC = PC;
            int target_PC = 0x100; // Just for example: branch target can be calculated
            bool branch_taken = false;  // Assume branch taken for this example, adjust as needed

            // Handle the control hazard (branch prediction)
            int cycle = clock_cycles;
            handleControlHazard(cycle, PC, branch_taken, branch_PC, target_PC);
        }

        // Output the instruction and cycle details (for debugging)
        cout << "Cycle " << clock_cycles << ": Executing instruction at PC = 0x" << hex << PC << " : " << instruction << "\n";
        clock_cycles++;
    }
}

vector<string> text_segment; // Stores text (instruction) segment
vector<string> data_segment; // Stores data (variables in .data)
map<string, string> memory;  // Stores final memory dump (address -> value)
unordered_map<int, int> execMemory;
unordered_map<int, int> pc_to_line;  // Maps PC value to line number


// Function to determine id .data type is word or other
// Convert register names (like x5) to 5-bit binary representation
string hex_to_binary(string hex_str)
{
    stringstream ss;
    ss << hex << hex_str;
    unsigned int n;
    ss >> n;
    return bitset<32>(n).to_string(); // Convert to 32-bit binary
}



string register_to_binary(string reg) {
    int reg_num = stoi(reg.substr(1)); // Remove 'x' and convert to number
    return bitset<5>(reg_num).to_string();
}

// Convert immediate values to binary with specified bit length
string immediate_to_binary(string imm, int bits) {
    int imm_num = stoi(imm);
    return bitset<32>(imm_num).to_string().substr(32 - bits);
}

// Convert a binary string to hexadecimal
string binary_to_hex(string binary_str) {
    bitset<32> b(binary_str);
    stringstream ss;
    ss << hex << uppercase << b.to_ulong();
    return ss.str();
}

void data_func(string data_type)
{
    if (data_type == "byte")
        factor = 1;
    else if (data_type == "half")
        factor = 2;
    else if (data_type == "word")
        factor = 4;
    else
        factor = 8;
}




// Function to convert PC into hexdecimal



void writeMemoryBlock(ofstream &output, string addr, string machineCode, string originalLine, string description) {
    output << addr << " " << machineCode 
           << " , " << originalLine 
           << " # " << description << "\n";
}

// Fetch stage: simply logs the instruction being fetched at a given PC

void fetch(const string& instruction) {
    cout << "Fetch Stage: Fetching instruction from instruction memory.\n";
    cout << "   >> " << instruction << "\n";
}


int main()
{
    // Prompt for Knob1 (Pipelining)
    promptKnobSetting("pipelining", Knob1);

    // Prompt for Knob2 (Data Forwarding)
    promptKnobSetting("data forwarding", Knob2);

    // Prompt for Knob3 (Print Register State)
    promptKnobSetting("printing register state at the end of each cycle", Knob3);

    // Prompt for Knob4 (Print Pipeline Registers)
    promptKnobSetting("printing pipeline registers at the end of each cycle", Knob4);

    // Prompt for Knob5 (Specific Instruction for Tracing)
    cout << "Enter the instruction number you want to track (enter -1 to disable): ";
    cin >> Knob5;
    if (Knob5 != -1) {
        cout << "Tracking instruction number " << Knob5 << ".\n";
    } else {
        cout << "No specific instruction tracking enabled.\n";
    }

    // Prompt for Knob6 (Print Branch Prediction Details)
    promptKnobSetting("printing branch prediction details", Knob6);

    // Proceed with the rest of the program and the pipeline logic...

    // Simulate pipeline execution (example)
    cout << "\nStarting simulation with the following settings:\n";
    cout << "Pipelining: " << (Knob1 ? "Enabled" : "Disabled") << "\n";
    cout << "Data Forwarding: " << (Knob2 ? "Enabled" : "Disabled") << "\n";
    cout << "Printing Register State: " << (Knob3 ? "Enabled" : "Disabled") << "\n";
    cout << "Printing Pipeline Registers: " << (Knob4 ? "Enabled" : "Disabled") << "\n";
    cout << "Tracking instruction number: " << (Knob5 == -1 ? "None" : to_string(Knob5)) << "\n";
    cout << "Branch Prediction Details: " << (Knob6 ? "Enabled" : "Disabled") << "\n";


    string binary_ans;
    // Opening the assembly file
    ifstream file("input.asm");
    if (!file.is_open())
    {
        cout << "Error opening file." << endl;
        return 0;
    }
    registers[2] = 0x7FFFFFE0; // Initialize stack pointer (x2) near top of memory
    // Taking every line of assembly code as strings in vector
    vector<string> lines;
    // Preprocessing: Split inline labels like "label: addi x1, x0, 5" into two separate lines
vector<string> processed_lines;
for (string& line : lines) {
    size_t colonPos = line.find(':');
    if (colonPos != string::npos && colonPos + 1 < line.size() && line[colonPos + 1] != ' ') {
        string label = line.substr(0, colonPos + 1);
        string rest = line.substr(colonPos + 1);
        processed_lines.push_back(label);
        processed_lines.push_back(rest);
    } else {
        processed_lines.push_back(line);
    }
}
lines = processed_lines; // Replace with cleaned version

    string s;
    while (getline(file, s))
    {
        lines.push_back(s);
    }
    // Completed taking file as input
    file.close();
    // Adding R format instructions
    Rf.push_back("add");
    Rf.push_back("and");
    Rf.push_back("or");
    Rf.push_back("sll");
    Rf.push_back("slt");
    Rf.push_back("sra");
    Rf.push_back("srl");
    Rf.push_back("sub");
    Rf.push_back("xor");
    Rf.push_back("mul");
    Rf.push_back("div");
    Rf.push_back("rem");
    // Adding I format Instruction
    If.push_back("addi");
    If.push_back("andi");
    If.push_back("ori");
    If.push_back("lb");
    If.push_back("ld");
    If.push_back("lh");
    If.push_back("lw");
    If.push_back("jalr");
    // Adding S format
    Sf.push_back("sb");
    Sf.push_back("sw");
    Sf.push_back("sd");
    Sf.push_back("sh");
    // Addig Sb format
    Sbf.push_back("beq");
    Sbf.push_back("bne");
    Sbf.push_back("bge");
    Sbf.push_back("blt");
    Sbf.push_back("ble");
    
    // Adding U format
    Uf.push_back("auipc");
    Uf.push_back("lui");
    // Adding UJ format
    Jf.push_back("jal");
    // END of inserting Mnemonics

    int flag = 0;
    // Creating output file
    ofstream output("output.mc");
    int counter_2 = 0;
    // Initializing map for label
    for (int i = 0; i < lines.size(); i++)
    {
        string ans = "";
        string Mnemonic = "";
        int j = 0;
        // Getting the opcode until space and avoiding space in from of the opcode
        while ((lines[i][j] != ' ' || Mnemonic == "") && j < lines[i].size())
        {
            // Checking if the instruction is comment or not
            if (lines[i][j] == '#')
            {
                flag++;
                break;
            }
            Mnemonic = Mnemonic + lines[i][j];
            j++;
        }
        // Checking if its a comment
        if (flag)
        {
            flag = 0;
            continue;
        }
        // If not a comment then proceeding with instruction
        if (Mnemonic == ".data:")
{
    i++;
    while ((lines[i] != ".text:") && i < lines.size())
    {
        string line = lines[i];
        // --- Label extraction before parsing .word/.asciiz ---
size_t colon_pos = line.find(':');
if (colon_pos != string::npos) {
    string label = line.substr(0, colon_pos);
    label.erase(remove_if(label.begin(), label.end(), ::isspace), label.end());
    m_label[label] = dat;
    line = line.substr(colon_pos + 1); // remove label part
    while (!line.empty() && isspace(line[0])) line.erase(0, 1); // clean whitespace
}

        if (line.empty()) { i++; continue; }

        int j = 0;
        while (j < line.size() && line[j] != '.') j++;
        j++;

        string data_type = "";
        while (j < line.size() && line[j] != ' ') {
            data_type += line[j];
            j++;
        }

        if (data_type == "asciiz" || data_type == "string") {
            string str = "";
            while (j < line.size() && line[j] != '"') j++;
            j++;

            while (j < line.size() && line[j] != '"') {
                if (line[j] == '\\' && j + 1 < line.size()) {
                    j++;
                    switch (line[j]) {
                        case 'n': str += '\n'; break;
                        case 't': str += '\t'; break;
                        case 'r': str += '\r'; break;
                        case '0': str += '\0'; break;
                        default: str += line[j]; break;
                    }
                } else {
                    str += line[j];
                }
                j++;
            }

            if (j >= line.size() || line[j] != '"') {
                cout << "Warning: Missing closing quote for string at line " << i + 1 << endl;
            }

            for (char ch : str) {
                string h = hexa_convert(int(ch));
                string addr = "0x" + hexa_convert(dat);
                string data_output = addr + " 0x" + h + " , " + line + " # Char: " + string(1, ch);
                data_segment.push_back(data_output);
                execMemory[dat] = int(ch);  // ✅ Store ASCII value
                dat++;
            }
        
            // ✅ Store null terminator at the end
            // After looping through all characters in the string `str`
string addr = "0x" + hexa_convert(dat);
string data_output = addr + " 0x0 , " + line + " # String termination";
data_segment.push_back(data_output);
execMemory[dat] = 0;  // ✅ Actually store the null byte
memory[addr] = "0x0"; // ✅ (if you're using this for display/memory dump)
dat++;

        }
        else {
            data_func(data_type); // Already sets factor = 1 for byte
            vector<string> arr;
            while (j < line.size() && line[j] == ' ') j++;
            while (j < line.size()) {
                string id = "";
                while (j < line.size() && line[j] != ',' && line[j] != ' ') {
                    id += line[j];
                    j++;
                }
                if (!id.empty()) arr.push_back(id);
                while (j < line.size() && (line[j] == ',' || line[j] == ' ')) j++;
            }
        
            for (int t = 0; t < arr.size(); t++) {
                long long val = stoll(arr[t], nullptr, 0); // supports decimal and hex like 0x01
            
                if (data_type == "byte" && (val < -128 || val > 255)) {
                    cerr << "Error: .byte value out of range (-128 to 255): " << val << " in line: " << line << endl;
                    exit(1);
                } else if (data_type == "half" && (val < -32768 || val > 65535)) {
                    cerr << "Error: .half value out of range (-32768 to 65535): " << val << " in line: " << line << endl;
                    exit(1);
                } else if (data_type == "word" && (val < INT32_MIN || val > UINT32_MAX)) {
                    cerr << "Error: .word value out of range (-2^31 to 2^32-1): " << val << " in line: " << line << endl;
                    exit(1);
                } else if (data_type == "dword" && (val < LLONG_MIN || static_cast<unsigned long long>(val) > ULLONG_MAX)) {
                    cerr << "Error: .dword value out of range (-2^63 to 2^64-1): " << val << " in line: " << line << endl;
                    exit(1);
                }
            
                string h = hexa_convert(val);
                string addr = "0x" + hexa_convert(dat);
                string data_output = addr + " 0x" + h + " , " + line + " # " + data_type + ": " + arr[t];
                data_segment.push_back(data_output);
            
                execMemory[dat] = val;
                memory[addr] = "0x" + h;
                dat += factor;
            }
            
        }
        
        i++;
    }
}

        else
        {

            // Creating map function for labels
            if (!counter_2) {
                int temp_PC = 0;
                for (int ot = i; ot < lines.size(); ot++) {
                    string line = lines[ot];
            
                    if (line.empty() || line[0] == '#') continue;
            
                    // ✅ Handle any label in the line (even inline)
                    size_t colon_pos = line.find(':');
                    if (colon_pos != string::npos) {
                        string label = line.substr(0, colon_pos);
                        label.erase(remove_if(label.begin(), label.end(), ::isspace), label.end());
                        m_label[label] = temp_PC;
            
                        // If there's more after the label on same line
                        string rest = line.substr(colon_pos + 1);
                        while (!rest.empty() && isspace(rest[0])) rest.erase(0, 1);
                        if (!rest.empty()) {
                            pc_to_line[temp_PC] = ot;
                            temp_PC += 4;
                        }
            
                        continue; // Skip further processing for this line
                    }
            
                    // Instruction line without label
                    if (line.find('.') == string::npos && !line.empty() && line[0] != '#') {
                        pc_to_line[temp_PC] = ot;
                        temp_PC += 4;
                    }
                }
            
                PC_2 = temp_PC;
                counter_2++;
            }
            
            // Checking if its R_format Mnemonics
            if (!Knob1 && ans == "")
{
    for (int k = 0; k < Rf.size(); k++)
    {
        if (Mnemonic == Rf[k]) {
            // --- Pipeline Stage 1: Fetch ---
            cout << "Cycle " << ++clock_cycles << ":\n";
            fetch(lines[i]);

            // --- Pipeline Stage 2: Decode ---
            string mnemonic;
            int rd, rs1, rs2;
            tie(mnemonic, rd, rs1, rs2) = decode_R(lines[i]);

            int rs1_val = registers[rs1];
            int rs2_val = registers[rs2];

            // --- Pipeline Stage 3: Execute ---
            int result = execute_R(mnemonic, rs1_val, rs2_val);

            // --- Pipeline Stage 4: Memory Access (no-op) ---
            memoryAccessR(mnemonic);

            // --- Pipeline Stage 5: Write Back ---
            writeBackR(rd, result, mnemonic);

            tie(ans, binary_ans) = R(lines[i], j, Mnemonic);
            string instruction_output = "0x" + hexa_convert(PC) + " " + ans + " , " + lines[i] + " # " + binary_ans;
            text_segment.push_back(instruction_output);

            PC += 4;
            break;
        }
    }
}



            // Checking if its I format Mnemonic
            if (!Knob1 && ans == "")
{
    for (int k = 0; k < If.size(); k++) {
        if (Mnemonic == If[k]) {
            int currentPC = PC;

            cout << "Cycle " << ++clock_cycles << ":\n";
            fetch(lines[i]);

            string mnemonic;
            int rd, rs1, imm;
            tie(mnemonic, rd, rs1, imm) = decode_I(lines[i], m_label);

            int rs1_val = registers[rs1];
            cout << "Decode Stage: Decoded I-type instruction.\n";
            cout << "   Mnemonic: " << mnemonic << ", rd (Ry): x" << rd << ", rs1: x" << rs1
                 << " = " << rs1_val << ", imm: " << imm << "\n";

            int aluResult = execute_I(mnemonic, rs1_val, imm);
            cout << "Execute Stage: ALU Result = " << aluResult << " → will be written to Ry (x" << rd << ")\n";

            int loadData = 0;
            memoryAccessI(mnemonic, aluResult, loadData);

            if (mnemonic == "lw" || mnemonic == "lb" || mnemonic == "lh" || mnemonic == "ld") {
                if (rd != 0) writeBackI(mnemonic, rd, loadData);
            } else {
                if (rd != 0) writeBackI(mnemonic, rd, aluResult);
            }

            tie(ans, binary_ans) = I(lines[i], j, Mnemonic);
            string instruction_output = "0x" + hexa_convert(currentPC) + " " + ans + " , " + lines[i] + " # " + binary_ans;
            text_segment.push_back(instruction_output);

            // ✅ Handle PC update
            if (mnemonic == "jalr") {
                int branch_PC = currentPC;
                int target_PC = aluResult;
                bool branchTaken = true;
                handleControlHazard(clock_cycles, PC, branchTaken, branch_PC, target_PC);

                if (aluResult % 4 != 0) {
                    cerr << "Runtime Error: Unaligned jump target address " << aluResult << " in jalr.\n";
                    exit(1);
                }
                if (pc_to_line.find(aluResult) != pc_to_line.end()) {
                    i = pc_to_line[aluResult] - 1;
                    PC = aluResult;
                } else {
                    cerr << "Runtime Error: Invalid jump target address " << aluResult << " in jalr.\n";
                    exit(1);
                }
            } else {
                PC += 4;
            }

            break;
        }
    }
}

            

            // Checking if its S format Mnemonic
            if (!Knob1 && ans == "")
{
    for (int k = 0; k < Sf.size(); k++)
    {
        if (Mnemonic == Sf[k]) {
            cout << "Cycle " << ++clock_cycles << ":\n";
            fetch(lines[i]);

            string mnemonic;
            int rs1, rs2, imm;
            tie(mnemonic, rs1, rs2, imm) = decode_S(lines[i]);

            int rs1_val = registers[rs1];
            int rs2_val = registers[rs2];

            int effectiveAddr = execute_S(mnemonic, rs1_val, imm);

            memoryAccessS(mnemonic, effectiveAddr, rs2_val);

            writeBackS(mnemonic);

            tie(ans, binary_ans) = S(lines[i], j, Mnemonic);
            string instruction_output = "0x" + hexa_convert(PC) + " " + ans + " , " + lines[i] + " # " + binary_ans;
            text_segment.push_back(instruction_output);

            PC += 4;
            break;
        }
    }
}


            // Checking if its SB format Mnemonics
            if (!Knob1 && ans == "")
{
    for (int k = 0; k < Sbf.size(); k++) {
        if (Mnemonic == Sbf[k]) {
            int originalPC = PC;

            cout << "Cycle " << ++clock_cycles << ":\n";
            fetch(lines[i]);

            string mnemonic;
            int rs1, rs2, offset;
            tie(mnemonic, rs1, rs2, offset) = decode_SB(lines[i], m_label, originalPC);

            int rs1_val = registers[rs1];
            int rs2_val = registers[rs2];

            bool branchTaken = execute_SB(mnemonic, rs1_val, rs2_val);
            int branch_PC = originalPC;
            int target_PC = originalPC + offset;
            handleControlHazard(clock_cycles, PC, branchTaken, branch_PC, target_PC);

            memoryAccessSB(mnemonic);

            writeBackSB(mnemonic, branchTaken, originalPC + offset);

            tie(ans, binary_ans) = SB(lines[i], j, Mnemonic, m_label, originalPC);
            string instruction_output = "0x" + hexa_convert(originalPC) + " " + ans + " , " + lines[i] + " # " + binary_ans;
            text_segment.push_back(instruction_output);

            if (branchTaken) {
                PC = originalPC + offset;

                if (pc_to_line.find(PC) != pc_to_line.end()) {
                    i = pc_to_line[PC] - 1;
                } else {
                    cerr << "Error: PC " << PC << " not found in pc_to_line map\n";
                    exit(1);
                }
            } else {
                PC = originalPC + 4;
            }

            break;
        }
    }
}

            // Checking if its U format Mnemonics
            // U-format
            if (!Knob1 && ans == "") {
                for (int k = 0; k < Uf.size(); k++) {
                    if (Mnemonic == Uf[k]) {
                        cout << "Cycle " << ++clock_cycles << ":\n";
                        fetch(lines[i]);
            
                        string mnemonic;
                        int rd;
                        long long imm;
                        tie(mnemonic, rd, imm) = decode_U(lines[i]);
            
                        int result = execute_U(mnemonic, imm, PC);
            
                        memoryAccessU(mnemonic);
            
                        writeBackU(mnemonic, rd, result);
            
                        tie(ans, binary_ans) = U(lines[i], j, Mnemonic);
                        string instruction_output = "0x" + hexa_convert(PC) + " " + ans + " , " + lines[i] + " # " + binary_ans;
                        text_segment.push_back(instruction_output);
            
                        PC += 4;
                        break;
                    }
                }
            }
            


            // Checking if its UJ format Mnemonics
            if (!Knob1 && ans == "")
{
    for (int k = 0; k < Jf.size(); k++) {
        if (Mnemonic == Jf[k]) {
            int currentPC = PC;  // ✅ Save current PC before modifying it

            cout << "Cycle " << ++clock_cycles << ":\n";
            fetch(lines[i]);

            string mnemonic;
            int rd, offset;
            tie(mnemonic, rd, offset) = decode_J(lines[i], m_label, currentPC);
            cout << "Decode Stage: Decoded J-type instruction.\n";
            cout << "   Mnemonic: " << mnemonic << ", rd (Ry): x" << rd << ", offset: " << offset << "\n";

            int targetAddress = execute_J(currentPC, offset);
            cout << "Execute Stage: Jump target address calculated as " << targetAddress << "\n";
            bool branchTaken = true;
            int branch_PC = currentPC;
            int target_PC = targetAddress;
            handleControlHazard(clock_cycles, PC, branchTaken, branch_PC, target_PC);

            memoryAccessJ(mnemonic);

            int returnAddress = currentPC + 4;  // ✅ Correct return address
            writeBackJ(rd, returnAddress, mnemonic);

            // Encoding & output instruction
            tie(ans, binary_ans) = J(lines[i], j, Mnemonic, m_label, currentPC);
            string instruction_output = "0x" + hexa_convert(currentPC) + " " + ans + " , " + lines[i] + " # " + binary_ans;
            text_segment.push_back(instruction_output);

            // ✅ Properly Update PC & i
            PC = targetAddress;

            if (pc_to_line.find(PC) != pc_to_line.end()) {
                i = pc_to_line[PC] - 1;
            } else {
                cerr << "Error: PC " << PC << " not found in pc_to_line map\n";
                exit(1);
            }

            break;
        }
    }
}

        }
    }
    // --- STEP 1: PRINT TEXT SEGMENT ---
output << "# --- TEXT SEGMENT ---\n";
for (const auto &entry : text_segment) {
    output << entry << "\n";
}
// Final termination
output << "0x" << hexa_convert(PC) << " Termination of code";
// --- STEP 2: PRINT DATA SEGMENT ---
output << "\n# --- DATA SEGMENT ---\n";
for (const auto &entry : data_segment) {
    output << entry << "\n";
}
// --- STEP 3: WRITE FINAL REGISTER STATE ---
ofstream regFile("register.txt");
if (!regFile.is_open()) {
    cerr << "Error: Could not open register.txt for writing.\n";
    return 1;
}

regFile << "# Final Register State\n";
for (int i = 0; i < 32; i++) {
    if (i == 2)
        regFile << "sp (x02) = " << registers[i] << "\n";
    else
        regFile << "x" << setw(2) << setfill('0') << i << " = " << registers[i] << "\n";
}

regFile.close();
ofstream memFile("memory.txt");
if (!memFile.is_open()) {
    cerr << "Error: Could not open memory.txt for writing.\n";
    return 1;
}

memFile << "# Memory Dump\n";
vector<int> memAddresses;
for (const auto& entry : execMemory) {
    memAddresses.push_back(entry.first);
}
sort(memAddresses.begin(), memAddresses.end());

for (int addr : memAddresses) {
    memFile << "0x" << hexa_convert(addr)
            << " = " << execMemory[addr] << "\n";
}
memFile.close();

if (Knob1) {
    lines.clear();
    ifstream file("input.asm");
while (getline(file, s)) lines.push_back(s);
file.close();

    int cycle = 0;


    // After all instructions have been fetched once, just drain the pipeline:
// PHASE 1: fetch & run until all instructions have been fetched
bool fetched_last_instruction = false;
// STEP 0: Preprocess and populate all labels with correct PC values
int temp_PC = 0;
for (int i = 0; i < lines.size(); i++) {
    string line = lines[i];

    if (line.empty() || line[0] == '#') continue;

    // Label on this line?
    size_t colon = line.find(':');
    if (colon != string::npos) {
        string label = line.substr(0, colon);
        label.erase(remove_if(label.begin(), label.end(), ::isspace), label.end());
        m_label[label] = temp_PC;

        // Check if there's an instruction after label
        string rest = line.substr(colon + 1);
        while (!rest.empty() && isspace(rest[0])) rest.erase(0, 1);
        if (!rest.empty()) {
            temp_PC += 4;
        }

        continue; // Don't process label again
    }

    if (line.find('.') == string::npos) {
        temp_PC += 4;
    }
}


while (PC < (int)lines.size() * 4 || IF_ID.valid || ID_EX.valid || EX_MEM.valid || MEM_WB.valid) {
    // Process stages in reverse order to simulate pipeline flow
    writeBackStage();
    memoryAccessStage();
    executeStage();
    instructionDecode();
    instructionFetch(lines, PC);

    // Always increment clock once per pipeline cycle
    clock_cycles++;
    
    if (data_hazard_stall_now) {
        cout << "[STALL DETECTED] Data hazard stall in this cycle.\n";
        data_hazard_stall_now = false;
    }
    if (control_hazard_stall_now) {
        cout << "[STALL DETECTED] Control hazard stall (branch misprediction) in this cycle.\n";
        control_hazard_stall_now = false;
    }
    if (Knob3) {
        printRegisterState();
    }
    
    
    if (Knob6) {
        cout << "BTB state after cycle " << clock_cycles << ":\n";
        for (const auto& entry : branch_prediction_table) {
            int pc = entry.first;
            bool prediction = entry.second;
    
            cout << "  0x" << hexa_convert(pc)
                 << " Prediction: " << (prediction ? "Taken" : "Not Taken")
                 << ", Target: ";
            if (branch_target_address.count(pc)) {
                cout << "0x" << hexa_convert(branch_target_address[pc]) << "\n";
            } else {
                cout << "N/A\n";
            }
        }
        cout << "-------------------------\n";
    }

    
    
}
ofstream regFile("register.txt");
if (!regFile.is_open()) {
    cerr << "Error: Could not open register.txt for writing.\n";
    return 1;
}

regFile << "# Final Register State\n";
for (int i = 0; i < 32; i++) {
    if (i == 2)
        regFile << "sp (x02) = " << registers[i] << "\n";
    else
        regFile << "x" << setw(2) << setfill('0') << i << " = " << registers[i] << "\n";
}

regFile.close();
ofstream memFile("memory.txt");
if (!memFile.is_open()) {
    cerr << "Error: Could not open memory.txt for writing.\n";
    return 1;
}}






ofstream stats("stats.txt");
stats << "Stat1: Total Cycles = " << clock_cycles << "\n";
stats << "Stat2: Total Instructions Executed = " << instruction_count << "\n";
stats << "Stat3: CPI = " << fixed << setprecision(2) << (float)clock_cycles / instruction_count << "\n";
stats << "Stat4: Data-transfer Instructions = " << data_transfer_count << "\n";
stats << "Stat5: ALU Instructions = " << alu_count << "\n";
stats << "Stat6: Control Instructions = " << control_count << "\n";
stats << "Stat7: Total Stalls/Bubbles = " << total_stalls << "\n";
stats << "Stat8: Data Hazards = " << data_hazard_count << "\n";
stats << "Stat9: Control Hazards = " << control_hazard_count << "\n";
stats << "Stat10: Branch Mispredictions = " << branch_mispredicts << "\n";
stats << "Stat11: Stalls due to Data Hazards = " << data_hazard_stalls << "\n";
stats << "Stat12: Stalls due to Control Hazards = " << control_hazard_stalls << "\n";
stats.close();
output.close(); 
}