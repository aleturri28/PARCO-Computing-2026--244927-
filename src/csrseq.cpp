#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <random>

using namespace std;

struct Triplet {
    int row;
    int col;
    double val;
};

bool compareTriplets(const Triplet& a, const Triplet& b) {
    if (a.row < b.row) return true;
    if (a.row == b.row) return a.col < b.col;
    return false;
}

struct CsrMatrix {
    int rows = 0;
    int cols = 0;
    int nnz  = 0;
    vector<int> row_ptr;
    vector<int> col_ind;
    vector<double> values;
};

// Sequential SpMV in CSR format
void spmv_csr_sequential(const CsrMatrix& A,
                         const vector<double>& v,
                         vector<double>& c) {
    const int* row_ptr   = A.row_ptr.data();
    const int* col_ind   = A.col_ind.data();
    const double* values = A.values.data();
    const double* v_in   = v.data();
    double* c_out        = c.data();

    const int rows = A.rows;

    for (int i = 0; i < rows; ++i) {
        double sum = 0.0;
        const int row_start = row_ptr[i];
        const int row_end   = row_ptr[i + 1];
        for (int j = row_start; j < row_end; ++j) {
            sum += values[j] * v_in[col_ind[j]];
        }
        c_out[i] = sum;
    }
}

// Helper: extract matrix name from full path
static string extract_matrix_name(const string& path) {
    size_t pos = path.find_last_of("/\\");
    string filename = (pos == string::npos) ? path : path.substr(pos + 1);

    const string ext = ".mtx";
    if (filename.size() > ext.size() &&
        filename.compare(filename.size() - ext.size(), ext.size(), ext) == 0) {
        filename.erase(filename.size() - ext.size(), ext.size());
    }
    return filename;
}

int main(int argc, char* argv[]) {
    // Expected CLI:
    //   ./spmv_seq <matrix.mtx>
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <matrix.mtx>\n";
        return 1;
    }

    string filename = argv[1];

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: could not open file " << filename << "\n";
        return 1;
    }

    vector<Triplet> triplets;
    int rows, cols, nnz;
    string line;

    // Skip comments
    while (getline(file, line)) {
        if (line.empty()) continue;
        if (line[0] != '%') break;
    }

    stringstream ss(line);
    ss >> rows >> cols >> nnz;
    triplets.reserve(nnz);

    int r, c;
    double v;
    for (int i = 0; i < nnz; ++i) {
        if (!(file >> r >> c >> v)) {
            cerr << "Error reading matrix data.\n";
            return 1;
        }
        Triplet t;
        t.row = r - 1;
        t.col = c - 1;
        t.val = v;
        triplets.push_back(t);
    }
    file.close();

    // Convert COO -> CSR
    sort(triplets.begin(), triplets.end(), compareTriplets);

    CsrMatrix csr;
    csr.rows = rows;
    csr.cols = cols;
    csr.nnz  = nnz;
    csr.row_ptr.assign(rows + 1, 0);
    csr.col_ind.resize(nnz);
    csr.values.resize(nnz);

    int current_row = 0;
    for (int i = 0; i < nnz; ++i) {
        csr.col_ind[i] = triplets[i].col;
        csr.values[i]  = triplets[i].val;

        while (current_row < triplets[i].row) {
            ++current_row;
            csr.row_ptr[current_row] = i;
        }
    }
    for (int i = current_row + 1; i <= rows; ++i) {
        csr.row_ptr[i] = nnz;
    }

    // Generate random input vector in [-1000, 1000]
    vector<double> v_input(cols);
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist(-1000.0, 1000.0);
    for (int i = 0; i < cols; ++i) {
        v_input[i] = dist(gen);
    }

    vector<double> c_output(rows, 0.0);

    // Warm-up run (not timed)
    spmv_csr_sequential(csr, v_input, c_output);

    // Timed runs
    const int NUM_RUNS = 10;
    vector<double> times_ms(NUM_RUNS);

    for (int run = 0; run < NUM_RUNS; ++run) {
        auto start = chrono::high_resolution_clock::now();
        spmv_csr_sequential(csr, v_input, c_output);
        auto end   = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> elapsed = end - start;
        times_ms[run] = elapsed.count();
    }

    string matrix_name = extract_matrix_name(filename);

    cout << matrix_name;
    for (int run = 0; run < NUM_RUNS; ++run) {
        cout << "," << times_ms[run];
    }
    cout << "\n";

    return 0;
}

