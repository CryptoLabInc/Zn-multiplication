// Copyright (c) 2026 HomomorphicEncryption.org
// All rights reserved.
//
// This software is licensed under the terms of the Apache v2 License.
// See the LICENSE.md file for details.

use indexmap::IndexMap;
use std::env;
use std::path::Path;
use std::fs;
use std::time::Instant;
use serde_json;

use tfhe::{ FheUint64, set_server_key, ServerKey };

use zn_multiplication::half_cipher_cipher_mul_64;

fn get_runtime(t: &Instant) -> f64 {
    (t.elapsed().as_nanos() as f64) * 1e-9
}


pub fn main() -> Result<(), Box<dyn std::error::Error>> {
    let time_start_server = Instant::now();
    
    // Get the number of inputs from the first argument
    let args: Vec<String> = env::args().collect();
    if args.len() < 3 {
        eprintln!("Usage: {} <size> <data size>", args[0]);
        std::process::exit(1); 
    }
    let size = args[1].clone();
    let data_size = args[2].parse::<usize>()?;
    let io_dir = "io/".to_owned() + &size;
    let report_path = io_dir.clone() + "/server_reported_steps.json";

    // Map to store profiling results
    let mut runtime_steps = IndexMap::new();

    // Load the server key
    let mut time_start = Instant::now();
    let serialised_data = fs::read(io_dir.clone() + "/public_keys/pk.bin")?;
    let server_key: ServerKey = bincode::deserialize(&serialised_data)?;
    set_server_key(server_key);
    runtime_steps.insert("Loading keys".to_string(),
                         get_runtime(&time_start));
 
    // Load the LHS input ciphers
    time_start = Instant::now();
    let ciphertexts_in_dir = io_dir.clone() + "/ciphertexts_upload";
    let ciphers_lhs = (0 .. data_size).map(|i|
        bincode::deserialize::<FheUint64>(&fs::read(ciphertexts_in_dir.clone() + "/cipher_lhs_" + &i.to_string() + ".bin")?)
    ).collect::<Result<Vec<FheUint64>, Box<bincode::ErrorKind>>>()?;

    
    // Load the RHS input ciphers
    let ciphers_rhs = (0 .. data_size).map(|i|
        bincode::deserialize::<FheUint64>(&fs::read(ciphertexts_in_dir.clone() + "/cipher_rhs_" + &i.to_string() + ".bin")?)
    ).collect::<Result<Vec<FheUint64>, Box<bincode::ErrorKind>>>()?;
    
    runtime_steps.insert("Loading inputs".to_string(),
                         get_runtime(&time_start));

    // Run the homomorphic multiplications
    time_start = Instant::now();
    let ciphers_out = ciphers_lhs.iter().zip(ciphers_rhs.iter())
                                 .map(|(lhs, rhs)| half_cipher_cipher_mul_64(lhs, rhs))
                                 .collect::<Vec<FheUint64>>();
    
    runtime_steps.insert("Homomorphic multiplications".to_string(),
                         get_runtime(&time_start));

    // Write the results
    time_start = Instant::now();
    let ciphertexts_out_dir = io_dir.clone() + "/ciphertexts_download";
    if !Path::new(&ciphertexts_out_dir).exists() {
        fs::create_dir(&ciphertexts_out_dir)?;
    }
    for (i, cipher) in ciphers_out.iter().enumerate() {
        fs::write(ciphertexts_out_dir.clone() + "/cipher_out_" + &i.to_string() + ".bin", &bincode::serialize(&cipher)?)?
    }
    runtime_steps.insert("Write results".to_string(),
                         get_runtime(&time_start));
    
    //Total server runtime
    runtime_steps.insert("Total".to_string(),
                         get_runtime(&time_start_server));

    // Write the report file
    let json = serde_json::to_string_pretty(&runtime_steps)?;
    fs::write(report_path, json)?;

    Ok(())
}
