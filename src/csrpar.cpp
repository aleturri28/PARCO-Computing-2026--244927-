#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <omp.h>

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

// Sequential SpMV in CSR format (used for warm-up or debugging)
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

// Parallel SpMV (CSR format) using schedule(runtime)
// The actual OpenMP schedule and chunk size are configured via omp_set_schedule()
// and omp_set_num_threads() in main().
void spmv_csr_parallel(const CsrMatrix& A,
                       const vector<double>& v,
                       vector<double>& c) {
    const int* row_ptr   = A.row_ptr.data();
    const int* col_ind   = A.col_ind.data();
    const double* values = A.values.data();
    const double* v_in   = v.data();
    double* c_out        = c.data();

    const int rows = A.rows;

    // Outer loop over rows is parallelized.
    // Each thread processes a subset of rows independently.
    #pragma omp parallel for schedule(runtime)
    for (int i = 0; i < rows; ++i) {
        double sum = 0.0;
        const int row_start = row_ptr[i];
        const int row_end   = row_ptr[i + 1];

        for (int j = row_start; j < row_end; ++j) {
            sum += values[j] * v_in[col_ind[j]];
        }

        // No race condition: each thread writes to a distinct c_out[i]
        c_out[i] = sum;
    }
}

// Helper: extract matrix name from full path
// Example: "/home/.../bcsstk17/bcsstk17.mtx" -> "bcsstk17"
static string extract_matrix_name(const string& path) {
    // Remove directory part
    size_t pos = path.find_last_of("/\\");
    string filename = (pos == string::npos) ? path : path.substr(pos + 1);

    // Remove ".mtx" extension if present
    const string ext = ".mtx";
    if (filename.size() > ext.size() &&
        filename.compare(filename.size() - ext.size(), ext.size(), ext) == 0) {
        filename.erase(filename.size() - ext.size(), ext.size());
    }
    return filename;
}

int main(int argc, char* argv[]) {
    // Expected CLI:
    //   ./spmv <matrix.mtx> <schedule_type> <chunk_size> <num_threads>
    if (argc != 5) {
        cerr << "Usage: " << argv[0]
             << " <matrix.mtx> <schedule_type> <chunk_size> <num_threads>\n";
        cerr << "  schedule_type: static, dynamic, guided\n";
        cerr << "  chunk_size:    e.g. 10, 100, 1000\n";
        cerr << "  num_threads:   e.g. 1, 2, 4, 8, 16\n";
        return 1;
    }

    string filename     = argv[1];
    string schedule_str = argv[2];
    int chunk_size      = 0;
    int num_threads     = 0;

    try {
        chunk_size  = stoi(argv[3]);
        num_threads = stoi(argv[4]);
    } catch (const std::exception&) {
        cerr << "Error: chunk_size and num_threads must be integer values.\n";
        return 1;
    }

    // Configure OpenMP schedule and number of threads
    omp_sched_t sched_kind;
    if (schedule_str == "static") {
        sched_kind = omp_sched_static;
    } else if (schedule_str == "dynamic") {
        sched_kind = omp_sched_dynamic;
    } else if (schedule_str == "guided") {
        sched_kind = omp_sched_guided;
    } else {
        cerr << "Error: invalid scheduling type. Use: static, dynamic, guided\n";
        return 1;
    }

    omp_set_num_threads(num_threads);
    omp_set_schedule(sched_kind, chunk_size);

    // --- Read Matrix Market file (.mtx) ---
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: could not open file " << filename << "\n";
        return 1;
    }

    vector<Triplet> triplets;
    int rows, cols, nnz;
    string line;

    // Skip comments starting with '%'
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
        // Convert from 1-based (MatrixMarket) to 0-based indices
        Triplet t;
        t.row = r - 1;
        t.col = c - 1;
        t.val = v;
        triplets.push_back(t);
    }
    file.close();

    // --- Convert COO -> CSR ---
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

    // --- Generate random input vector v in [-1000, 1000] ---
    vector<double> v_input(cols);
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist(-1000.0, 1000.0);
    for (int i = 0; i < cols; ++i) {
        v_input[i] = dist(gen);
    }

    vector<double> c_output(rows, 0.0);

    // --- Warm-up run (not timed, just to stabilize caches / OpenMP runtime) ---
    spmv_csr_parallel(csr, v_input, c_output);

    // --- Timed runs ---
    const int NUM_RUNS = 10;
    vector<double> times_ms(NUM_RUNS);

    for (int run = 0; run < NUM_RUNS; ++run) {
        double start = omp_get_wtime();
        spmv_csr_parallel(csr, v_input, c_output);
        double end   = omp_get_wtime();
        times_ms[run] = (end - start) * 1000.0; // milliseconds
    }

    // --- CSV output (single line) ---
    // Format (B1): matrix,schedule,chunk,threads,run1,...,run10
    string matrix_name = extract_matrix_name(filename);

    cout << matrix_name << "," << schedule_str << ","
         << chunk_size << "," << num_threads;

    for (int run = 0; run < NUM_RUNS; ++run) {
        cout << "," << times_ms[run];
    }
    cout << "\n";

    return 0;
}

