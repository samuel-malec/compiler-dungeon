#pragma once

#include <cstring>
#include <iostream>
#include <string>
#include <stdexcept>

namespace dungeon {
enum class pipeline_stage {
  full,
  lexer,
  parser,
  semantic,
  typecheck,
  hir,
  ir,
  ssa,
  optimization,
  interpret,
};

inline pipeline_stage parse_stage(std::string_view stage_name) {
  if (stage_name == "full" || stage_name == "all")
    return pipeline_stage::full;
  if (stage_name == "lexer" || stage_name == "lex")
    return pipeline_stage::lexer;
  if (stage_name == "parser" || stage_name == "parse")
    return pipeline_stage::parser;
  if (stage_name == "semantic" || stage_name == "sema")
    return pipeline_stage::semantic;
  if (stage_name == "typecheck" || stage_name == "type-check")
    return pipeline_stage::typecheck;
  if (stage_name == "hir")
    return pipeline_stage::hir;
  if (stage_name == "ir")
    return pipeline_stage::ir;
  if (stage_name == "ssa")
    return pipeline_stage::ssa;
  if (stage_name == "interpret")
    return pipeline_stage::interpret;
  throw std::runtime_error("unknown stage: " + std::string(stage_name));
}

struct config {
  std::string in_name;
  std::string out_name;
  bool emit_tokens;
  bool emit_ast;
  bool emit_hir;
  bool emit_ir;
  bool emit_ssa;
  pipeline_stage stage = pipeline_stage::full;
  bool verify_each;
  bool interpret;
  bool interpret_each;
  bool emit_mir;
  bool emit_asm;
  bool out_of_ssa;
};

inline void help() {
  // TODO: think which of these flags should be optional and which should be turned on by default
  std::cout << "Usage:\n"
      << "./compiler-dungeon file.ks [--stage lexer|parser|semantic|typecheck|hir|ir|full] [-o output]\n"
      << "--emit-tokens\n"
      << "--emit-ast\n"
      << "--emit-hir\n"
      << "--emit-ir\n"
      << "--emit-ssa\n"
      << "--verify-each\n"
      << "--interpret\n"
      << "--interpret-each\n"
      << "--out-of-ssa (run the out-of-SSA pass and print the IR)\n"
      << "--emit-mir\n"
      << "--emit-asm (writes RISC-V assembly to the -o file, a.s by default)\n";
}

inline config parse_config(int argc, char* const* argv) {
  if (argc < 1)
    throw std::runtime_error("Usage: ./compiler-dungeon file.ks [--stage ...]\n");

  if (strcmp(argv[0], "-h") == 0) {
    help();
    exit(0);
  }

  std::string file_in = argv[0];
  std::string file_out = "a.s";
  bool _emit_tokens = false;
  bool _emit_ast = false;
  bool _emit_hir = false;
  bool _emit_ir = false;
  bool _emit_ssa = false;
  bool _verify_each = false;
  bool _interpret = false;
  bool _interpret_each = false;
  bool _emit_mir = false;
  bool _emit_asm = false;
  bool _out_of_ssa = false;
  auto stage = pipeline_stage::full;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-o") == 0) {
      if (i >= argc - 1)
        throw std::runtime_error("missing argument of -o");

      ++i;
      file_out = argv[i];
    } else if (strcmp(argv[i], "--stage") == 0) {
      if (i >= argc - 1)
        throw std::runtime_error("missing argument of --stage");
      ++i;
      stage = parse_stage(argv[i]);
    } else if (std::strncmp(argv[i], "--stage=", 8) == 0) {
      stage = parse_stage(argv[i] + 8);
    } else if (strcmp(argv[i], "--emit-tokens") == 0) {
      _emit_tokens = true;
    } else if (strcmp(argv[i], "--emit-ast") == 0) {
      _emit_ast = true;
    } else if (strcmp(argv[i], "--emit-hir") == 0) {
      _emit_hir = true;
    } else if (strcmp(argv[i], "--emit-ir") == 0) {
      _emit_ir = true;
    } else if (strcmp(argv[i], "--emit-ssa") == 0) {
      _emit_ssa = true;
    } else if (strcmp(argv[i], "--verify-each") == 0) {
      _verify_each = true;
    } else if (strcmp(argv[i], "--interpret") == 0) {
      _interpret = true;
    } else if (strcmp(argv[i], "--interpret-each") == 0) {
      _interpret_each = true;
    } else if (strcmp(argv[i], "--out-of-ssa") == 0) {
      _out_of_ssa = true;
    } else if (strcmp(argv[i], "--emit-mir") == 0) {
      _emit_mir = true;
    } else if (strcmp(argv[i], "--emit-asm") == 0) {
      _emit_asm = true;
    } else {
      throw std::runtime_error(std::string("invalid flag: ") + argv[i]);
    }
  }

  return {
      .in_name = file_in,
      .out_name = file_out,
      .emit_tokens = _emit_tokens,
      .emit_ast = _emit_ast,
      .emit_hir = _emit_hir,
      .emit_ir = _emit_ir,
      .emit_ssa = _emit_ssa,
      .stage = stage,
      .verify_each = _verify_each,
      .interpret = _interpret,
      .interpret_each = _interpret_each,
      .emit_mir = _emit_mir,
      .emit_asm = _emit_asm,
      .out_of_ssa = _out_of_ssa
  };
}

}
