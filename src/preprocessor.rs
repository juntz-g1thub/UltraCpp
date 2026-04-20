use serde::{Deserialize, Serialize};
use std::collections::HashSet;
use std::fs;
use std::path::{Path, PathBuf};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ImportInfo {
    pub module_path: String,
    pub alias: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PreprocessedModule {
    pub source: String,
    pub imports: Vec<ImportInfo>,
    pub exports: Vec<String>,
}

pub struct Preprocessor {
    include_paths: Vec<PathBuf>,
    visited_files: HashSet<PathBuf>,
}

impl Preprocessor {
    pub fn new() -> Self {
        Preprocessor {
            include_paths: vec![PathBuf::from(".")],
            visited_files: HashSet::new(),
        }
    }

    pub fn add_include_path(&mut self, path: &str) {
        self.include_paths.push(PathBuf::from(path));
    }

    pub fn preprocess(&mut self, source: &str, file_path: Option<&Path>) -> PreprocessedModule {
        let mut imports = Vec::new();
        let mut exports = Vec::new();
        let mut output = Vec::new();

        let source_dir = file_path.and_then(|p| p.parent()).map(|p| p.to_path_buf());

        if let Some(ref dir) = source_dir {
            self.include_paths.insert(0, dir.clone());
        }

        for line in source.lines() {
            let trimmed = line.trim();

            if trimmed.starts_with("#include") {
                let content = self.handle_include(trimmed, source_dir.as_deref());
                output.push(content);
            } else if trimmed.starts_with("#import") {
                if let Some(import_info) = self.handle_import(trimmed) {
                    imports.push(import_info);
                }
                output.push(String::new());
            } else if trimmed.starts_with("export ") {
                if let Some(export_name) = self.extract_export_name(trimmed) {
                    exports.push(export_name);
                }
                output.push(line.to_string());
            } else {
                output.push(line.to_string());
            }
        }

        PreprocessedModule {
            source: output.join("\n"),
            imports,
            exports,
        }
    }

    fn handle_include(&mut self, line: &str, source_dir: Option<&Path>) -> String {
        if let Some(path) = self.extract_string_content(line) {
            let full_path = self.resolve_include_path(&path, source_dir);

            if full_path.exists() {
                if let Ok(content) = fs::read_to_string(&full_path) {
                    let included_source = content.replace("\n", "\n  ");
                    return format!(
                        "  // Begin #include \"{}\"\n{}\n  // End #include",
                        path, included_source
                    );
                }
            }
            format!("  // #include \"{}\" - file not found", path)
        } else {
            String::new()
        }
    }

    fn handle_import(&mut self, line: &str) -> Option<ImportInfo> {
        let content = line.trim_start_matches("#import").trim();

        let module_path = if content.starts_with('<') {
            let start = content.find('<')? + 1;
            let end = content.find('>')?;
            if start < end {
                content[start..end].to_string()
            } else {
                return None;
            }
        } else if content.starts_with('"') {
            self.extract_string_content(line)?
        } else {
            return None;
        };

        let alias = if let Some(alias_start) = content.rfind(" as ") {
            let alias = content[alias_start + 4..].trim();
            Some(alias.to_string())
        } else {
            None
        };

        Some(ImportInfo { module_path, alias })
    }

    fn extract_string_content(&self, line: &str) -> Option<String> {
        let start = line.find('"')? + 1;
        let end = line.rfind('"')?;
        if start < end {
            Some(line[start..end].to_string())
        } else {
            None
        }
    }

    fn resolve_include_path(&self, path: &str, source_dir: Option<&Path>) -> PathBuf {
        if let Ok(full_path) = fs::canonicalize(path) {
            return full_path;
        }

        if let Some(dir) = source_dir {
            let full_path = dir.join(path);
            if full_path.exists() {
                return full_path;
            }
        }

        for include_path in &self.include_paths {
            let full_path = include_path.join(path);
            if full_path.exists() {
                return full_path;
            }
        }

        PathBuf::from(path)
    }

    fn extract_export_name(&self, line: &str) -> Option<String> {
        let decl = line.trim_start_matches("export").trim();

        if decl.starts_with("int")
            || decl.starts_with("void")
            || decl.starts_with("char")
            || decl.starts_with("float")
            || decl.starts_with("double")
            || decl.starts_with("bool")
        {
            if let Some(paren_pos) = decl.find('(') {
                let before_paren = &decl[..paren_pos];
                let parts: Vec<&str> = before_paren.split_whitespace().collect();
                if parts.len() >= 2 {
                    return Some(parts[parts.len() - 1].to_string());
                }
            }
        }

        None
    }
}

impl Default for Preprocessor {
    fn default() -> Self {
        Self::new()
    }
}

impl PreprocessedModule {
    pub fn write_dep_file(&self, output_path: &Path) -> Result<(), std::io::Error> {
        let json = serde_json::json!({
            "module": output_path.file_stem()
                .and_then(|s| s.to_str())
                .unwrap_or("unnamed"),
            "file": output_path.to_string_lossy(),
            "imports": self.imports,
            "exports": self.exports
        });

        fs::write(output_path, serde_json::to_string_pretty(&json).unwrap())?;
        Ok(())
    }
}
