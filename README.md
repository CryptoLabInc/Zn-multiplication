# FHE Benchmarking Suite - 64-bits multiplication
This is a submission for the FHE benchmarking suite by CryptoLab Inc. using the scheme described in the paper [FHE for SIMD Arithmetic Logic Units with Amortized O(1) Bootstrapping per Ciphertext](https://eprint.iacr.org/2026/233).

## Benchmark Results
The benchmark results were measured on **AWS EC2 i7ie.metal-24xl**. 

We provide the results with step-by-step breakdown here because the harness does not separate the runtime of compute and I/O operations, and the latter may vary across the runs. 

The breakdown was obtained by adding logging in the submission code to measure the runtime of individual steps. One can reproduce the breakdown by running the submission as usual.

All times are in milliseconds (ms).
### Single Instances

**Common steps** 

| Phase | Step | |
| --- | --- | ---: |
| KeyGen | Compute | 13.4 |
| | Write | 0.45 |
| Encryption | Read input texts | 0.04 |
| | Setup | 0.15 |
| | Encode | 14.5 |
| | Encrypt | 6.2 |
| | Write ciphertexts | 1.2 |

**Per-run steps**

| Phase | Step | Run 1 | Run 2 | Run 3 |
| --- | --- | ---: | ---: | ---: |
| Compute | Read relin key | 4.02 | 4.27 | 4.15 |
| | Setup | 16.32 | 20.73 | 17.76 |
| | Read ciphertexts | 1.62 | 1.87 | 1.55 |
| | Compute | 12.42 | 13.17 | 13.15 |
| | Write result ciphertexts | 0.68 | 0.75 | 0.68 |
| Decryption | Read secret key | 0.14 | 0.12 | 0.10 |
| | Setup | 0.007 | 0.006 | 0.005 |
| | Read result ciphertexts | 0.84 | 0.65 | 0.56 |
| | Decrypt | 5.60 | 5.93 | 5.40 |
| | Decode | 6.45 | 6.98 | 6.10 |
| | Write output text | 0.003 | 0.004 | 0.003 |

### Small Instances

**Common steps** 

| Phase | Step | Time (ms) |
| --- | --- | ---: |
| KeyGen | Compute | 13.3 |
| | Write | 0.36 |
| Encryption | Read input texts | 0.18 |
| | Setup | 0.15 |
| | Encode | 15.1 |
| | Encrypt | 6.4 |
| | Write ciphertexts | 1.0 |

**Per-run steps**

| Phase | Step | Run 1 | Run 2 | Run 3 |
| --- | --- | ---: | ---: | ---: |
| Compute | Read relin key | 4.17 | 4.25 | 4.12 |
| | Setup | 16.92 | 20.29 | 19.37 |
| | Read ciphertexts | 1.57 | 1.71 | 1.66 |
| | Compute | 12.59 | 13.45 | 13.13 |
| | Write result ciphertexts | 0.59 | 0.68 | 0.67 |
| Decryption | Read secret key | 0.15 | 0.13 | 0.11 |
| | Setup | 0.007 | 0.007 | 0.006 |
| | Read result ciphertexts | 0.81 | 0.72 | 0.71 |
| | Decrypt | 5.66 | 5.61 | 6.35 |
| | Decode | 7.11 | 7.39 | 7.46 |
| | Write output text | 0.09 | 0.10 | 0.09 |

### Medium Instances

**Common steps** 

| Phase | Step | Time (ms) |
| --- | --- | ---: |
| KeyGen | Compute | 13.4 |
| | Write | 0.51 |
| Encryption | Read input texts | 12.5 |
| | Setup | 0.17 |
| | Encode | 207.7 |
| | Encrypt | 166.4 |
| | Write ciphertexts | 226.5 |

**Per-run steps**

| Phase | Step | Run 1 | Run 2 | Run 3 |
| --- | --- | ---: | ---: | ---: |
| Compute | Read relin key | 4.41 | 4.51 | 4.35 |
| | Setup | 63.9 | 64.5 | 65.5 |
| | Read ciphertexts | 208.1 | 207.4 | 207.4 |
| | Compute | 92.9 | 91.5 | 91.3 |
| | Write result ciphertexts | 131.6 | 950.3 | 1333.4 |
| Decryption | Read secret key | 0.14 | 0.14 | 0.14 |
| | Setup | 0.006 | 0.007 | 0.008 |
| | Read result ciphertexts | 102.2 | 96.8 | 105.9 |
| | Decrypt | 59.2 | 84.3 | 58.9 |
| | Decode | 86.9 | 77.6 | 83.0 |
| | Write output text | 7.0 | 6.9 | 6.9 |

### Large Instances

**Common steps**

| Phase | Step | Time (ms) |
| --- | --- | ---: |
| KeyGen | Compute | 14.2 |
| | Write | 0.52 |
| Encryption | Read input texts | 1117.6 |
| | Setup | 0.20 |
| | Encode | 14540 |
| | Encrypt | 12718 |
| | Write ciphertexts | 18157 |

**Per-run steps**

| Phase | Step | Run 1 | Run 2 | Run 3 |
| --- | --- | ---: | ---: | ---: |
| Compute | Read relin key | 4.41 | 16.07 | 4.41 |
| | Setup | 63.1 | 64.7 | 63.9 |
| | Read ciphertexts | 15912 | 15976 | 16252 |
| | Compute | 5937 | 6169 | 6177 |
| | Write result ciphertexts | 42594 | 207179 | 221982 |
| Decryption | Read secret key | 0.14 | 0.14 | 0.17 |
| | Setup | 0.008 | 0.006 | 0.007 |
| | Read result ciphertexts | 7132 | 7069 | 7155 |
| | Decrypt | 3662 | 3656 | 3539 |
| | Decode | 6591 | 6755 | 6535 |
| | Write output text | 902 | 693 | 695 |

## Execution Modes

The 64-bits workload currently only support local execution mode:

All steps are executed on a single machine:
- Cryptographic context setup
- Key generation
- Input preprocessing and encryption
- Homomorphic multiplication
- Decryption and postprocessing

## Running the 64-bits multiplication workload

#### Dependencies
- [Miniforge](https://github.com/conda-forge/miniforge) (conda package manager)
- Python 3.12+ with `numpy`
- CMake 3.23+
- gcc 14.3.0
- HEaaN2 library (pre-built for `x86_64` Linux, provided in `submission/install/`)

All dependencies (except HEaaN2) can be installed via conda using the provided `environment.yml`.

> **Note:** The bundled HEaaN2 library is compiled for the `x86_64` architecture on Linux. Running the workload on other architectures (e.g. `aarch64`/Apple Silicon) is not supported.

#### Installing Miniforge
If you don't already have conda, install Miniforge first. On Linux `x86_64`:
```console
curl -O https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh
bash ~/Miniconda3-latest-Linux-x86_64.sh
source ~/.bashrc
```
You can optionally run `conda init` to have conda available in new shells automatically.

#### Execution
To run the workload, clone the repository and set up the environment:
```console
git clone https://github.com/fhe-benchmarking/Zn-multiplication.git
cd Zn-multiplication

conda env create -f environment.yml
conda activate zn-multiplication

python3 harness/run_submission.py -h  # Information about command-line options
```

The harness script `harness/run_submission.py` will build the submission using CMake on the first run (via `scripts/build_task.sh`). Subsequent runs reuse the already-built binaries.

```console
$ python3 harness/run_submission.py -h
usage: run_submission.py [-h] [--num_runs NUM_RUNS] [--seed SEED] [--clrtxt CLRTXT] {0,1,2,3}

Run the 64-bits mul FHE benchmark.

positional arguments:
  {0,1,2,3}            Instance size (0-single/1-small/2-medium/3-large)

options:
  -h, --help           show this help message and exit
  --num_runs NUM_RUNS  Number of times to run steps 5-7 (default: 1)
  --seed SEED          Random seed for dataset generation
  --clrtxt CLRTXT      Specify with 1 if to rerun the cleartext computation
```

The single instance runs the multiplication for a single pair of inputs and verifies the correctness of the result.

```console
$ python3 ./harness/run_submission.py 0 --seed 3 --num_runs 2

[harness] Running submission for single dataset
-- Found OpenMP_CXX: -fopenmp (found version "4.5")
-- Found OpenMP_CXX: -fopenmp (found version "4.5")
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: ../submission/build
[ 20%] Built target Zn-multiplication
[ 33%] Built target client_preprocess_input
[ 46%] Built target client_postprocess
[ 66%] Built target client_encode_encrypt_input
[ 80%] Built target client_key_generation
[ 86%] Built target server_encrypted_compute
[100%] Built target client_decrypt_decode
16:50:40 [harness] 1: Input generation completed (elapsed: 0.1612s)
16:50:40 [harness] 2: Input preprocessing completed (elapsed: 0.0008s)
16:50:40 [harness] 3: Key Generation completed (elapsed: 0.0338s)
         [harness] Public and evaluation keys size: 1.5M
16:50:40 [harness] 4: Input encryption completed (elapsed: 0.0381s)
         [harness] Encrypted input size: 4.0M

         [harness] Run 1 of 2
16:50:40 [harness] 5: Encrypted computation completed (elapsed: 0.067s)
         [harness] Encrypted results size: 2.0M
16:50:40 [harness] 6: Result decryption completed (elapsed: 0.0556s)
16:50:40 [harness] 7: Result postprocessing completed (elapsed: 0.0007s)
[harness] PASS
[total latency] 0.3571s

         [harness] Run 2 of 2
16:50:40 [harness] 5: Encrypted computation completed (elapsed: 0.1622s)
         [harness] Encrypted results size: 2.0M
16:50:40 [harness] 6: Result decryption completed (elapsed: 0.0237s)
16:50:40 [harness] 7: Result postprocessing completed (elapsed: 0.0006s)
[harness] PASS
[total latency] 0.4203s

All steps completed for the single dataset!
```

## Directory structure

The directory structure of this reposiroty is as follows:
```
├─ README.md     # This file
├─ LICENSE.md    # Harness software license (Apache v2)
├─ harness/      # Scripts to drive the workload implementation
|   ├─ generate_dataset.py
|   ├─ params.py
|   ├─ run_submission.py
|   ├─ utils.py
|   └─ [,,,]
├─ datasets/     # The harness scripts create and populate this directory
├─ docs/         # Optional: additional documentation
├─ io/           # This directory is used for client<->server communication
├─ measurements/ # Holds logs with performance numbers
├─ scripts/      # Helper scripts for dependencies and build system
└─ submission/   # This is where the workload implementation lives
    ├─ README.md   # Submission documentation (mandatory)
    ├─ LICENSE.md  # Software license (different from Apache v2)
    └─ [...]
```
Submitters must overwrite the contents of the `scripts` and `submissions`
subdirectories.

## Description of stages

A submitter can edit any of the files in `/submission`. 
Moreover, for the particular parameters related to a workload, the submitter can modify the `harness/params.py` files.
If the current description of the files are inaccurate, the stage names in `harness/run_submission.py` can be also 
modified.

The order in which they are happening in `run_submission` assumes an initialization step which run only once, and potentially multiple runs for the multiplication.
Each file can take as argument the test case size.

***


| Stage executables                | Description |
|----------------------------------|-------------|
| `client_key_generation`          | Generate all key material and cryptographic context.           
| `client_preprocess_input`        | (Optional) Any in the clear computations the client wants to apply over the input.
| `client_encode_encrypt_input`    | Plaintext encoding and encryption of the input.
| `server_encrypted_compute`       | The computation the server applies to achieve the workload solution over encrypted data.
| `client_decrypt_decode`          | Decryption and plaintext decoding of the result at the client.
| `client_postprocess`             | Any in the clear computation that the client wants to apply on the decrypted result.


The outer python script measures the runtime of each stage.
The current stage separation structure requires reading and writing to files more times than minimally necessary.
For a more granular runtime measuring, which would account for the extra overhead described above, we encourage
submitters to separate and print in a log the individual times for reads/writes and computations inside each stage. 
