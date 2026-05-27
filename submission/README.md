# Workload implementation—64-bits multiplication

This is a submission for the 64-bits multiplication workload by CryptoLab Inc., written in C++ and using the [HEaaN2](https://heaan.io) library. The implementation is based on the scheme described in the paper [FHE for SIMD Arithmetic Logic Units with Amortized O(1) Bootstrapping per Ciphertext](https://eprint.iacr.org/2026/233).

## Security and Parameters

The submission uses the parameter with the following configuration:

| Parameter | Value |
|-----------|-------|
| Ring degree | 2^16 (65536) |
| Secret key Hamming weight | 32 |
| Error distribution | Discrete Gaussian (σ = 3.2) |
| log(PQ) | 114 |

Accodring to [sparse-key-estimate](https://github.com/jdumezy/sparse-key-estimate/blob/master/Precomputed-Tables/128bits_security.md), these configurations provide 128 bits of security in the IND-CPA model.


## Build and run

Building the submission requires **CMake 3.23+**, **GCC 14.3.0**, and the **HEaaN2 library** (pre-built for `x86_64` Linux, provided under `submission/install/`).

> **Note:** The bundled HEaaN2 library is compiled for the `x86_64` architecture on Linux. Other architectures (e.g. `aarch64`/Apple Silicon) are not supported.

The recommended way to install the build toolchain and Python dependencies is via conda using the top-level `environment.yml`. If you don't already have conda, install [Miniforge](https://github.com/conda-forge/miniforge) first. On Linux `x86_64`:
```console
curl -L -O "https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-x86_64.sh"
bash Miniforge3-Linux-x86_64.sh -b -p "$HOME/miniforge3"
source "$HOME/miniforge3/etc/profile.d/conda.sh"
```

**Build**

From the repository root, run:
```console
bash scripts/build_task.sh
```

This configures and builds all stage executables via CMake:
```console
cd submission
cmake -B build -DBUILD_WITH_CUDA=OFF -DCMAKE_PREFIX_PATH=./install
cmake --build build -j $(nproc)
```

Binaries are placed in `submission/target/release/`.

**Run**

The submission is driven by the harness. From the repository root:
```console
pip install -r requirements.txt
python3 harness/run_submission.py <size>
```

where `<size>` is one of `0` (single), `1` (small), `2` (medium), or `3` (large). See the top-level [README](../README.md) for a full example run.


## Executables

| Executable | Description |
|---|---|
| `client_key_generation` | Generates secret key, and relinearization key. |
| `client_preprocess_input` | Placeholder; no cleartext preprocessing required. |
| `client_encode_encrypt_input` | Encodes 64-bit words via `WordEncoder` and `EnDecoder` and encrypts them. |
| `server_encrypted_compute` | Performs encrypted multiplication with relinearization and rescaling. |
| `client_decrypt_decode` | Decrypts and decodes ciphertexts back to 64-bit word vectors. |
| `client_postprocess` | Placeholder; no cleartext postprocessing required. |

