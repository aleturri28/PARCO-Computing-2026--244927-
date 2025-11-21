# PARCO Computing 2026 – SpMV Performance Evaluation Project

**Author:** Alessandro Turri
**Course:** Parallel Computing 2025/2026 (updated README) of Trento

---

# 1. Project Overview

This project evaluates **Sparse Matrix–Vector Multiplication (SpMV)** using the **CSR (Compressed Sparse Row)** format.
Both **sequential** and **parallel (OpenMP)** implementations are analysed across multiple sparse matrices with different sparsity patterns.

The goals of the project are:

* Compare **sequential** vs **parallel** SpMV
* Evaluate **thread scaling** from 2 to 64 threads
* Compare **OpenMP scheduling**: `static`, `dynamic`, `guided`, `auto`
* Analyse the effect of **chunk sizes**: 10, 100, 1000
* Measure **performance counters** (with `perf`)
* Analyse **cache behaviour** (with Valgrind Cachegrind)
* Ensure **complete reproducibility** of all experiments

---

# 2. Matrices Used

### Manual download example (MatrixMarket .mtx parsing)

Below is an example of how a single SuiteSparse matrix can be manually downloaded and extracted:

```
wget https://suitesparse-collection-website.herokuapp.com/MM/HB/1138_bus.tar.gz
gzip -d 1138_bus.tar.gz
tar -xf 1138_bus.tar
ls 1138_bus/1138_bus.mtx
```

The matrices used in this project are:

The following SuiteSparse matrices are used for benchmarking:

| Matrix       | nnz        | rows      | cols      | Notes                    |
| ------------ | ---------- | --------- | --------- | ------------------------ |
| **heart2**   | 680,341    | 2,339     | 2,339     | Structural problem       |
| **olm2000**  | 7,996      | 2,000     | 2,000     | Chemical engineering     |
| **bcsstk17** | 428,650    | 10,974    | 10,974    | Stiffness matrix         |
| **msc10848** | 1,229,776  | 10,848    | 10,848    | Medium FEM matrix        |
| **hcircuit** | 513,072    | 105,676   | 105,676   | Irregular sparsity       |
| **x104**     | 8,713,602  | 108,384   | 108,384   | Circuit simulation       |
| **cont1_l**  | 7,031,999  | 1,918,399 | 1,921,596 | Contact mechanics        |
| **cage14**   | 27,130,349 | 1,505,785 | 1,505,785 | Very large sparse matrix |

*(All nnz/rows/cols extracted from SuiteSparse.)* and dimensions will be added after parsing matrices.)*

Place all matrices in:

```
matrix/<name>/<name>.mtx
```

---

# 3. Repository Structure

```
repo/
├── README.md
├── src/
│   ├── csrseq.cpp            # Sequential CSR implementation
│   └── csrpar.cpp            # Parallel CSR implementation (OpenMP)
│
├── scripts/
│   ├── run_seq.pbs           # Sequential PBS job
│   ├── run_csrpar.pbs        # Parallel PBS job
│   ├── run_perf.pbs          # Perf profiling
│   └── run_cachegrind_seq.pbs # Cachegrind profiling
│
├── matrix/
│   └── default.txt           # Placeholder (actual matrices downloaded externally)
│
├── results/
│   └── default.txt           # Placeholder for benchmark results
│
├── plots/
│   └── default.txt           # Placeholder for generated plots
│
├── spmv                      # Parallel executable (compiled manually)
├── spmv_seq                  # Sequential executable (compiled manually)
└── .git/                     # Git metadata
```
````

---

# 4. Compilation

Compilation is done **manually** outside PBS scripts.

### Sequential Version

```bash
g++ -std=c++11 -o spmv_seq src/csrseq.cpp
````

### Parallel Version (OpenMP)

```bash
g++ -std=c++11 -fopenmp -o spmv src/csrpar.cpp
```

---

# 5. Running on the HPC Cluster (UniTN)

All experiments were executed on **public queue ****************************************************************************`short_cpuQ`**:

* Walltime limit: **6 hours**
* Up to **64 threads** available
* Suitable for repeated benchmarking

Execution via PBS scripts:

```bash
qsub -v MATRIX_NAME=<matrix_name> run_seq.pbs
qsub -v MATRIX_NAME=<matrix_name> run_csrpar.pbs
qsub -v MATRIX_NAME=<matrix_name> run_perf.pbs
qsub -v MATRIX_NAME=<matrix_name> run_cachegrind_seq.pbs
```

Example:

```bash
qsub -v MATRIX_NAME=heart2 run_csrpar.pbs
```

---

# 6. Experimental Methodology

For each matrix:

* **Sequential:** 10 runs → mean + 90th percentile
* **Parallel:** warm-up + 10 timed runs per configuration

Parameters tested:

* **Threads:** 2, 4, 8, 16, 32, 64
* **Scheduling:** static, dynamic, guided
* **Chunk sizes:** 10, 100, 1000

Instrumentation:

* `perf stat` → instructions, cycles, IPC, cache misses, branches
* Valgrind Cachegrind → detailed L1/L2/LLC behavior

---

# 7. Running Locally

### Sequential

```bash
./spmv_seq matrix/heart2/heart2.mtx
```

### Parallel

```bash
export OMP_NUM_THREADS=8
export OMP_SCHEDULE="static,10"
./spmv matrix/heart2/heart2.mtx
```

### Run all matrices

```bash
for m in matrix/*/*.mtx; do ./spmv_seq "$m"; done
```

---

# 8. Results

Collected results include:

* Execution times (mean, 90th percentile)
* Speedup and efficiency curves
* Cache miss analysis
* IPC and cycle counts
* Scheduling policy comparison

Results are stored in:

```
results/
```

Plots and figures in:

```
plots/
```

---

# 9. Discussion Summary

### **Key Findings**

* SpMV is **memory‑bound**, not compute‑bound
* Small matrices (e.g., heart2) show limited parallel speedup
* Large matrices (e.g., cage14) benefit strongly from parallelism
* **Static scheduling** works best for regular sparsity
* **Dynamic/guided** works best for irregular matrices (e.g., hcircuit)
* Performance saturates around **32–64 threads**, limited by memory bandwidth

---

# 10. Reproducibility Setup # 10. Reproducibility Setup Script

The following script automates cloning the repository, downloading all matrices used in this project, compiling both sequential and parallel versions, and (if running on the UniTN HPC cluster) submitting PBS jobs automatically.

```bash
echo "=== 1) Clone repository ==="
git clone https://github.com/aleturri28/PARCO-Computing-2026-244927-.git PARCO-Computing-2026-244927- || exit 1

echo "=== 2) Download SuiteSparse matrices ==="
cd matrix

for url in \
  "https://sparse.tamu.edu/MM/Norris/heart2.tar.gz" \
  "https://sparse.tamu.edu/MM/Bai/olm2000.tar.gz" \
  "https://sparse.tamu.edu/MM/HB/bcsstk17.tar.gz" \
  "https://sparse.tamu.edu/MM/Boeing/msc10848.tar.gz" \
  "https://sparse.tamu.edu/MM/Hamm/hcircuit.tar.gz" \
  "https://sparse.tamu.edu/MM/DNVS/x104.tar.gz" \
  "https://sparse.tamu.edu/MM/vanHeukelum/cage14.tar.gz" \
  "https://sparse.tamu.edu/MM/Mittelmann/cont1_l.tar.gz" \
do
  fname=$(basename "$url")
  dirname="${fname%.tar.gz}"

  echo "Downloading $fname ..."
  wget "$url"

  echo "Extracting $fname ..."
  tar -xzf "$fname"

  echo "Cleaning archive..."
  rm "$fname"
done

cd ..

echo "=== 3) Compile sequential code ==="
g++ -std=c++11 src/csrseq.cpp -o spmv_seq

echo "=== 4) Compile parallel code (OpenMP) ==="
g++ -std=c++11 -fopenmp src/csrpar.cpp -o spmv

echo "=== 5) Submitting PBS jobs (only on HPC UniTN) ==="

if [[ "$HOSTNAME" == *"hpc"* ]]; then
    echo "Detected HPC environment — submitting jobs..."

    qsub -v MATRIX_NAME=heart2 scripts/run_seq.pbs
    qsub -v MATRIX_NAME=heart2 scripts/run_csrpar.pbs
    qsub -v MATRIX_NAME=heart2 scripts/run_perf.pbs
    qsub -v MATRIX_NAME=heart2 scripts/run_cachegrind_seq.pbs

    echo "All jobs submitted!"
else
    echo "You are NOT on the HPC cluster. Run these manually after ssh:"
    echo ""
    echo "Sequential:"
    echo "  qsub -v MATRIX_NAME=<matrix> scripts/run_seq.pbs"
    echo "  qsub -v MATRIX_NAME=<matrix> scripts/run_cachegrind_seq.pbs"
    echo ""
    echo "Parallel:"
    echo "  qsub -v MATRIX_NAME=<matrix> scripts/run_csrpar.pbs"
    echo "  qsub -v MATRIX_NAME=<matrix> scripts/run_perf.pbs"
fi

echo "=== SETUP COMPLETE ==="
```

# 11. Additional Reproducibility Notest automates cloning the repository, downloading all matrices used in this project, compiling both sequential and parallel versions, and (if running on the UniTN HPC cluster) submitting PBS jobs automatically.

```bash
echo "=== 1) Clone repository ==="
git clone https://github.com/aleturri28/PARCO-Computing-2026-244927-.git
cd repo || exit 1

echo "=== 2) Download SuiteSparse matrices ==="
cd matrix

for url in \
  "https://sparse.tamu.edu/MM/Norris/heart2.tar.gz" \
  "https://sparse.tamu.edu/MM/Bai/olm2000.tar.gz" \
  "https://sparse.tamu.edu/MM/HB/bcsstk17.tar.gz" \
  "https://sparse.tamu.edu/MM/Boeing/msc10848.tar.gz" \
  "https://sparse.tamu.edu/MM/Hamm/hcircuit.tar.gz" \
  "https://sparse.tamu.edu/MM/DNVS/x104.tar.gz" \
  "https://sparse.tamu.edu/MM/vanHeukelum/cage14.tar.gz" \
  "https://sparse.tamu.edu/MM/Mittelmann/cont1_l.tar.gz" \
do
  fname=$(basename "$url")
  dirname="${fname%.tar.gz}"

  echo "Downloading $fname ..."
  wget "$url"

  echo "Extracting $fname ..."
  tar -xzf "$fname"

  echo "Cleaning archive..."
  rm "$fname"
done

cd ..

echo "=== 3) Compile sequential code ==="
g++ -std=c+11  src/csrseq.cpp -o spmv_seq

echo "=== 4) Compile parallel code (OpenMP) ==="
g++ -std=c++11 -fopenmp src/csrpar.cpp -o spmv

echo "=== 5) Submitting PBS jobs (only on HPC UniTN) ==="

if [[ "$HOSTNAME" == *"hpc"* ]]; then
    echo "Detected HPC environment — submitting jobs..."

    qsub -v MATRIX_NAME=heart2 scripts/run_seq.pbs
    qsub -v MATRIX_NAME=heart2 scripts/run_csrpar.pbs
    qsub -v MATRIX_NAME=heart2 scripts/run_perf.pbs
    qsub -v MATRIX_NAME=heart2 scripts/run_cachegrind_seq.pbs

    echo "All jobs submitted!"
else
    echo "You are NOT on the HPC cluster. Run these manually after ssh:"
    echo ""
    echo "Sequential:"
    echo "  qsub -v MATRIX_NAME=<matrix> scripts/run_seq.pbs"
    echo "  qsub -v MATRIX_NAME=<matrix> scripts/run_cachegrind_seq.pbs"
    echo ""
    echo "Parallel:"
    echo "  qsub -v MATRIX_NAME=<matrix> scripts/run_csrpar.pbs"
    echo "  qsub -v MATRIX_NAME=<matrix> scripts/run_perf.pbs"
fi

echo "=== SETUP COMPLETE ==="

- All runs use a fixed seed (`srand(42)`) for reproducibility
- Matrix dimensions must follow:

```

matrix//.mtx

```

- Matrices larger than 100 MB are not stored in GitHub; instructions provided in README

---

# 👤 Author

**Alessandro Turri**\
Parallel Computing 2025/2026 – University of Trento

```

