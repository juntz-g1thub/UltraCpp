use clap::Parser;
use std::fs;
use std::path::PathBuf;
use std::process;

mod codegen;
mod error;
mod frontend;
mod preprocessor;
mod semantic;

use codegen::generator::CodeGenerator;
use frontend::{compile, dump_ast};

#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    #[arg(help = "Input source file (.uc or .upp)")]
    input: PathBuf,

    #[arg(short = 'o', long, help = "Output .ll file")]
    output: Option<PathBuf>,

    #[arg(short = 'a', long, help = "Print AST (debug)")]
    dump_ast: bool,

    #[arg(short = 'i', long, help = "Print LLVM IR")]
    dump_ir: bool,

    #[arg(short, long, help = "Compile to executable (requires llc + gcc)")]
    compile: bool,

    #[arg(long, help = "Print tokens and exit (no codegen, no preprocess)")]
    dump_tokens: bool,
}

fn main() {
    let args = Args::parse();

    let source = match fs::read_to_string(&args.input) {
        Ok(s) => s,
        Err(e) => {
            eprintln!("Error: Cannot read file '{}': {}", args.input.display(), e);
            process::exit(1);
        }
    };

    if args.dump_tokens {
        frontend::dump_tokens(&source);
        return;
    }

    if args.dump_ast {
        dump_ast(&source);
        return;
    }

    let module_name = args
        .input
        .file_stem()
        .and_then(|s| s.to_str())
        .unwrap_or("unnamed")
        .to_string();

    match compile(&source, Some(&args.input)) {
        Ok(result) => {
            let mut generator = CodeGenerator::new(&module_name);

            for import in &result.imports {
                generator.add_imported_module(&import.module_path);
            }

            match generator.generate(&result.ast) {
                Ok(ir) => {
                    let output_path = args.output.clone().or_else(|| {
                        args.input
                            .file_stem()
                            .map(|s| PathBuf::from(s.to_string_lossy().to_string() + ".ll"))
                    });

                    if let Some(path) = &output_path {
                        if let Err(e) = fs::write(path, &ir) {
                            eprintln!("Error writing IR: {}", e);
                            process::exit(1);
                        }
                        println!("Wrote IR to {}", path.display());
                    }

                    if args.dump_ir {
                        println!("=== LLVM IR ===");
                        println!("{}", ir);
                    }

                    if args.compile {
                        if let Some(ll_path) = &output_path {
                            let stem = ll_path.to_string_lossy().replace(".ll", "");

                            let asm_path = format!("{}.s", stem);
                            let obj_path = format!("{}.o", stem);
                            let exe_path = stem.clone();

                            let status = std::process::Command::new("llc")
                                .args(&[ll_path.to_str().unwrap(), "-o", &asm_path])
                                .status();

                            if !status.map(|s| s.success()).unwrap_or(false) {
                                eprintln!("llc failed");
                                process::exit(1);
                            }
                            println!("Compiled to {}", asm_path);

                            let status = std::process::Command::new("gcc")
                                .args(&[&asm_path, "-o", &exe_path])
                                .status();

                            if !status.map(|s| s.success()).unwrap_or(false) {
                                eprintln!("gcc linking failed");
                                process::exit(1);
                            }
                            println!("Linked to {}", exe_path);
                        }
                    }

                    println!("Done");
                }
                Err(e) => {
                    eprintln!("Error generating code: {}", e);
                    process::exit(1);
                }
            }
        }
        Err(e) => {
            eprintln!("Error: {}", e);
            process::exit(1);
        }
    }
}
