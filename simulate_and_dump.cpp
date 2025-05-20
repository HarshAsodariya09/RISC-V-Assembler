// simulate_and_dump.cpp
#include <bits/stdc++.h>
#include <fstream>
#include <iostream>
#include <unordered_map>
using namespace std;

// Assuming your simulation code declares and uses a global register file:
unordered_map<int, int> registers; // keys: register numbers (0-31), value: content

// Prototype for your simulation main function (or you can integrate its code here).
// For example, if your simulation code is in main() of your existing file,
// you can copy/paste it here. For demonstration, we use a stub simulation.
void runSimulation() {
    // --- BEGIN SIMULATION CODE ---
    // (Your entire simulation code goes here.)
    // For demonstration purposes, let's update some registers:
    registers[0] = 0;   // x0 is always 0
    registers[1] = 5;   // For example, x1 = 5 (could be updated during simulation)
    registers[5] = 42;  // Some computed value
    registers[10] = 100; // And so on...
    // --- END SIMULATION CODE ---
}

// Function to dump final register values into a file
void dumpRegisters(const string &filename = "registers.txt") {
    ofstream regFile(filename);
    if (!regFile.is_open()) {
        cerr << "Error opening " << filename << endl;
        return;
    }
    // Dump registers x0 through x31
    for (int i = 0; i < 32; i++) {
        // Ensure x0 remains 0
        if (i == 0)
            regFile << "x0 0" << "\n";
        else
            regFile << "x" << i << " " << registers[i] << "\n";
    }
    regFile.close();
    cout << "Final register values dumped to " << filename << "\n";
}

int main() {
    // Run your simulation code.
    runSimulation();

    // At the end of the simulation, dump the final register values.
    dumpRegisters();

    // Optionally, print simulation output to stdout as well.
    // (You could also dump other simulation details to files if desired.)
    return 0;
}
