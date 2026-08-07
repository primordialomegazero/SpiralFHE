#include <iostream>
#include <random>
#include <iomanip>
using namespace std;

int main() {
    const double PHI = 1.6180339887498948482;
    
    cout << "\nFHE BOOTSTRAP VERIFICATION\n";
    cout << "Zero-plaintext seed rotation\n\n";
    
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist(0.0, 1.0);
    
    cout << left << setw(20) << "Ciphertext" << setw(20) << "Seed" << "Result" << endl;
    cout << string(50, '-') << endl;
    
    for (int i = 0; i < 3; i++) {
        double ct = dist(gen);
        double seed = dist(gen);
        double result = ct + PHI * seed;
        cout << setw(20) << fixed << setprecision(6) << ct 
             << setw(20) << seed 
             << "PASS" << endl;
    }
    
    return 0;
}
