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
| KeyGen | Compute | 13.5 |
| | Write | 0.45 |
| Encryption | Read input texts | 0.04 |
| | Setup | 0.14 |
| | Encode | 13.5 |
| | Encrypt | 5.3 |
| | Write ciphertexts | 1.1 |

**Per-run steps**

| Phase | Step | Run 1 | Run 2 | Run 3 |
| --- | --- | ---: | ---: | ---: |
| Compute | Read relin key | 3.3 | 4.4 | 3.3 |
| | Setup | 20.6 | 20.0 | 21.7 |
| | Read ciphertexts | 1.4 | 1.3 | 1.1 |
| | Compute | 10.6 | 10.3 | 10.5 |
| | Write result ciphertexts | 0.61 | 0.67 | 0.58 |
| Decryption | Read secret key | 0.12 | 0.14 | 0.10 |
| | Setup | 0.007 | 0.007 | 0.006 |
| | Read result ciphertexts | 0.66 | 0.68 | 0.52 |
| | Decrypt | 5.1 | 5.2 | 4.4 |
| | Decode | 5.3 | 5.5 | 5.6 |
| | Write output text | 0.003 | 0.003 | 0.002 |

### Small Instances

**Common steps** 

| Phase | Step | Time (ms) |
| --- | --- | ---: |
| KeyGen | Compute | 13.7 |
| | Write | 0.43 |
| Encryption | Read input texts | 0.17 |
| | Setup | 0.13 |
| | Encode | 14.2 |
| | Encrypt | 5.3 |
| | Write ciphertexts | 1.1 |

**Per-run steps**

| Phase | Step | Run 1 | Run 2 | Run 3 |
| --- | --- | ---: | ---: | ---: |
| Compute | Read relin key | 3.4 | 3.5 | 3.1 |
| | Setup | 19.9 | 21.1 | 20.6 |
| | Read ciphertexts | 1.8 | 1.4 | 1.2 |
| | Compute | 10.1 | 10.4 | 10.4 |
| | Write result ciphertexts | 0.62 | 0.68 | 0.59 |
| Decryption | Read secret key | 0.12 | 0.11 | 0.11 |
| | Setup | 0.006 | 0.006 | 0.004 |
| | Read result ciphertexts | 0.68 | 0.53 | 0.51 |
| | Decrypt | 5.09 | 5.03 | 5.34 |
| | Decode | 5.9 | 5.8 | 6.57 |
| | Write output text | 0.07 | 0.07 | 0.07 |

### Medium Instances

**Common steps** 

| Phase | Step | Time (ms) |
| --- | --- | ---: |
| KeyGen | Compute | 13.1 |
| | Write | 0.45 |
| Encryption | Read input texts | 11.8 |
| | Setup | 0.15 |
| | Encode | 218 |
| | Encrypt | 160 |
| | Write ciphertexts | 219 |

**Per-run steps**

| Phase | Step | Run 1 | Run 2 | Run 3 |
| --- | --- | ---: | ---: | ---: |
| Compute | Read relin key | 3.6 | 3.8 | 3.8 |
| | Setup | 65.6 | 66 | 65.8 |
| | Read ciphertexts | 200 | 201 | 202 |
| | Compute | 94.3 | 88.7 | 91.3 |
| | Write result ciphertexts | 129 | 951 | 1340 |
| Decryption | Read secret key | 0.15 | 0.14 | 0.16 |
| | Setup | 0.005 | 0.008 | 0.008 |
| | Read result ciphertexts | 96.1 | 92.0 | 102 |
| | Decrypt | 67.66 | 52.11 | 52.54 |
| | Decode | 102.77 | 105.7 | 104.74 |
| | Write output text | 5.7 | 5.7 | 5.7 |

### Large Instances

**Common steps**

| Phase | Step | Time (ms) |
| --- | --- | ---: |
| KeyGen | Compute | 15.5 |
| | Write | 0.53 |
| Encryption | Read input texts | 1113 |
| | Setup | 0.19 |
| | Encode | 15778 |
| | Encrypt | 12252 |
| | Write ciphertexts | 18561 |

**Per-run steps**

| Phase | Step | Run 1 | Run 2 | Run 3 |
| --- | --- | ---: | ---: | ---: |
| Compute | Read relin key | 3.9 | 3.8 | 3.8 |
| | Setup | 68.9 | 70.97 | 71.1 |
| | Read ciphertexts | 16093 | 16331 | 16421 |
| | Compute | 6586 | 6544 | 6617 |
| | Write result ciphertexts | 42758 | 201392 | 224175 |
| Decryption | Read secret key | 0.15 | 0.16 | 0.17 |
| | Setup | 0.008 | 0.008 | 0.006 |
| | Read result ciphertexts | 7419 | 7226 | 7082 |
| | Decrypt | 3381 | 3404 | 3348 |
| | Decode | 7749 | 8356 | 7726 |
| | Write output text | 801 | 570 | 571 |

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
