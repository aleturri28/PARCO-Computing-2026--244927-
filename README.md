# PARCO Computing 2026 – SpMV Performance Evaluation Project

**Author:** Alessandro Turri
**Course:** Parallel Computing 2025/2026 – University of Trento

---

## 1. Project Overview

This project evaluates **Sparse Matrix–Vector Multiplication (SpMV)** using the **CSR (Compressed Sparse Row)** format.
Both **sequential** and **parallel (OpenMP)** implementations are analysed across multiple sparse matrices with different sparsity patterns.

Main objectives:

* Compare **sequential** vs **parallel** SpMV
* Evaluate **thread scaling** from 2 to 64 threads
* Compare **OpenMP scheduling** policies: `static`, `dynamic`, `guided`
* Analyse the effect of **chunk sizes**: 10, 100, 1000
* Measure **performance counters** with `perf`
* Analyse **cache behaviour** with Valgrind **Cachegrind**
* Provide a **fully reproducible** experimental setup

---

## 2. Matrices Used

### Manual download example (MatrixMarket .mtx parsing)

Example of how a single SuiteSparse matrix can be manually downloaded and extracted:

```bash
wget https://suitesparse-collection-website.herokuapp.com/MM/HB/1138_bus.tar.gz
gzip -d 1138_bus.tar.gz
tar -xf 1138_bus.tar
ls 1138_bus/1138_bus.mtx
```

The matrices used in this project are:

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

Place all matrices in:

```bash
matrix/<name>/<name>.mtx
```

---

## 3. Repository Structure

```bash
repo/
├── README.md
├── src/
│   ├── csrseq.cpp             # Sequential CSR implementation
│   └── csrpar.cpp             # Parallel CSR implementation (OpenMP)
│
├── scripts/
│   ├── run_seq.pbs            # Sequential PBS job
│   ├── run_csrpar.pbs         # Parallel PBS job
│   ├── run_perf.pbs           # Perf profiling
│   └── run_cachegrind_seq.pbs # Cachegrind profiling
│
├── matrix/
│   └── default.txt            # Placeholder (matrices downloaded externally)
│
├── results/
│   └── default.txt            # Placeholder for benchmark results
│
├── plots/
│   └── default.txt            # Placeholder for generated plots
│
├── spmv                       # Parallel executable (compiled manually)
├── spmv_seq                   # Sequential executable (compiled manually)
└── .git/                      # Git metadata
```

---

## 4. Compilation

Compilation is done **manually**, outside PBS scripts.

### Sequential Version

```bash
g++ -std=c++11 -O3 -o spmv_seq src/csrseq.cpp
```

### Parallel Version (OpenMP)

```bash
g++ -std=c++11 -O3 -fopenmp -o spmv src/csrpar.cpp
```

---

## 5. Running on the HPC Cluster (UniTN)

All experiments were executed on the public queue **`short_cpuQ`**:

* Walltime limit: **6 hours**
* Up to **64 threads** available
* Suitable for repeated benchmarking runs

Execution via PBS scripts (from inside `repo/scripts/`):

```bash
qsub -v MATRIX_NAME=<matrix_name> run_seq.pbs
qsub -v MATRIX_NAME=<matrix_name> run_csrpar.pbs
qsub -v MATRIX_NAME=<matrix_name> run_perf.pbs
qsub -v MATRIX_NAME=<matrix_name> run_cachegrind_seq.pbs
```

Example:

```bash
qsub -v MATRIX_NAME=heart2 scripts/run_csrpar.pbs
```

---

## 6. Experimental Methodology

For each matrix:

* **Sequential:** 10 runs → report mean and 90th percentile
* **Parallel:** one warm-up run + 10 timed runs per configuration

Parameters explored:

* **Threads:** 2, 4, 8, 16, 32, 64
* **Scheduling:** `static`, `dynamic`, `guided`
* **Chunk sizes:** 10, 100, 1000

Instrumentation:

* `perf stat` → instructions, cycles, IPC, cache misses, branches
* Valgrind **Cachegrind** → detailed L1/L2/LLC behavior

---

## 7. Running Locally

### Sequential

```bash
./spmv_seq matrix/heart2/heart2.mtx
```

### Parallel

Usage: `./spmv <matrix_path> <schedule> <chunk> <threads>`

Example running with 8 threads, static scheduling, and chunk size 10:

```bash
./spmv matrix/heart2/heart2.mtx static 10 8
```
---

## 8. Results

Collected results include:

* Execution times
* Cache miss

Results are stored in:

```bash
results/
```

Plots and figures are stored in:

```bash
plots/
```

---

## 9. Discussion Summary

### Key Findings

* SpMV is **memory‑bound**, not compute‑bound
* Small matrices (e.g., **heart2**) show limited parallel speedup
* Large matrices (e.g., **cage14**) benefit significantly from parallelism
* **Static scheduling** works best for regular sparsity patterns
* **Dynamic** / **guided** scheduling work better for irregular matrices (e.g., **hcircuit**)
* Performance saturates around **32–64 threads**, limited by memory bandwidth

---

## 10. Reproducibility Setup Script

The script below automates cloning the repository, downloading all matrices used in this project, compiling both sequential and parallel versions, and (if running on the UniTN HPC cluster) submitting PBS jobs automatically.

```bash
### === 1) Clone repository ===
git clone https://github.com/aleturri28/PARCO-Computing-2026--244927-.git repo
cd repo || exit 1


### === 2) Download SuiteSparse matrices ===
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
; do
    fname=$(basename "$url")
    tarname="${fname%.gz}"
    dirname="${tarname%.tar}"

    echo "Downloading $fname ..."
    wget "$url"

    echo "Extracting ..."
    gzip -d "$fname"
    tar -xf "$tarname"

    echo "Organizing matrix folder $dirname ..."
    mkdir -p "$dirname"
    find "$dirname" -name "*.mtx" -exec mv {} "$dirname" \;

    rm "$tarname"
    echo "=== Done $dirname ==="
done

cd ..


### === 3) Compile sequential code ===
g++ -std=c++11 -O3 src/csrseq.cpp -o spmv_seq


### === 4) Compile parallel OpenMP code ===
g++ -std=c++11 -O3 -fopenmp src/csrpar.cpp -o spmv


### === 5) Submit PBS jobs with job-limit protection ===
cd scripts

matrices=(heart2 olm2000 bcsstk17 msc10848 hcircuit x104 cage14 cont1_l)

submit_and_wait() {
    M=$1
    echo "Submitting 4 jobs for $M ..."

    qsub -v MATRIX_NAME=$M run_seq.pbs
    qsub -v MATRIX_NAME=$M run_csrpar.pbs
    qsub -v MATRIX_NAME=$M run_perf.pbs
    qsub -v MATRIX_NAME=$M run_cachegrind_seq.pbs

    echo "Waiting until job count < 28 ..."
    while true; do
        NJOBS=$(qstat -u "$USER" | grep -c "$USER")
        if [ "$NJOBS" -lt 28 ]; then
            break
        fi
        sleep 5
    done
}

echo "Detected HPC environment — submitting jobs sequentially with waiting..."
for M in "${matrices[@]}"; do
    submit_and_wait "$M"
done

echo "=== ALL JOBS SUBMITTED SAFELY ==="

```

---

## Author

**Alessandro Turri**
Parallel Computing 2025/2026 – University of Trento

