#include "compiler.hpp"

#include "progress_reporter.hpp"
#include "../analysis/ir_verifier.hpp"
#include "../analysis/pass_manager.hpp"
#include "../analysis/pipeline.hpp"
#include "../common/file.hpp"
#include "../common/pretty_printer.hpp"

#include "../frontend/parser.hpp"
#include "../sema/semantic.hpp"
#include "../frontend/token.hpp"

#include "../hir/ast2hir.hpp"
#include "../ir/hir2ir.hpp"
#include "../ir/interpreter.hpp"
#include "../backend/emit_rv64.hpp"
#include "../backend/isel.hpp"
#include "../backend/spill_all.hpp"

#include <fstream>

namespace dungeon {
void compiler::run(config& conf) {
  std::string in_name = conf.in_name;
  std::string out_name = conf.out_name;
  diag::source_ptr doc = std::make_shared<diag::source_file>(in_name, read_file(in_name));
  print::pretty_printer printer{};
  progress_reporter reporter{};
  std::vector<token> toks{};
  {
    reporter.time("lexing");
    lexer l{doc};
    toks = l.lex();
  }
  if (conf.emit_tokens || conf.stage == pipeline_stage::lexer)
    print::pretty_printer::print_tokens(toks);
  if (conf.stage == pipeline_stage::lexer)
    return;

  parser p{std::move(toks)};
  std::optional<ast::module> ast = std::nullopt;
  {
    reporter.time("parsing");
    ast = p.parse_module();
  }
  if (!ast)
    throw std::runtime_error("parser failed");
  if (conf.emit_ast || conf.stage == pipeline_stage::parser)
    printer.print_ast_module(std::cout, ast.value());
  if (conf.stage == pipeline_stage::parser)
    return;

  sema::semantic_analyzer sa{};
  {
    reporter.time("semantic analysis");
    sa.run(ast.value());
  }
  if (conf.stage == pipeline_stage::semantic || conf.stage == pipeline_stage::typecheck)
    return;

  hir::module hir{};
  {
    reporter.time("hir lowering");
    hir = hir::lower_ast_to_hir(ast.value(), sa.semantics);
  }
  if (conf.emit_hir || conf.stage == pipeline_stage::hir)
    printer.print_hir_module(std::cout, hir);
  if (conf.stage == pipeline_stage::hir)
    return;

  ir::module ir_module{};
  {
    reporter.time("generating ir");
    ir_module = ir::lower_hir_to_ir(hir, sa.semantics);
  }
  if (conf.emit_ir || conf.stage == pipeline_stage::ir)
    printer.print_ir_module(std::cout, ir_module);
  if (conf.stage == pipeline_stage::ir)
    return;

  std::optional<ir::exec_result> interpreter_res = std::nullopt;
  std::function<void(pass&, ir::function&)> check_behaviour;
  {
    reporter.time("building cfg");
    ir::cfg_builder builder{};
    builder.build(ir_module);
    if (conf.verify_each)
      for (auto& fn : ir_module.funcs)
        ir_verifier::verify_or_throw(fn, "cfg_builder");

    if (conf.interpret_each)
      interpreter_res = ir::interpreter{ir_module}.run(sa.semantics);
  }

  {
    reporter.time("analysis");
    analysis::pass_manager pm = get_default_pipeline();

    if (conf.interpret_each)
      check_behaviour = [&](pass& p, ir::function& fn) {
        const auto now = ir::interpreter{ir_module}.run(sa.semantics);
        if (now != *interpreter_res)
          throw std::runtime_error(
              "behaviour changed after '" + p.description() + "' on function #" +
              std::to_string(&fn - ir_module.funcs.data()) +
              "\n  before: " + ir::to_string(*interpreter_res) +
              "\n  after:  " + ir::to_string(now));
      };
    pm.after_pass = check_behaviour;
    for (auto& fn : ir_module.funcs)
      pm.run(fn, conf.verify_each);
  }

  if (conf.emit_ssa || conf.stage == pipeline_stage::ssa) {
    printer.print_ssa_ir(std::cout, ir_module);
  }

  if (conf.interpret || conf.stage == pipeline_stage::interpret) {
    ir::interpreter inter{ir_module};
    std::cout << ir::to_string(inter.run(sa.semantics)) << '\n';
  }

  if (conf.out_of_ssa || conf.emit_asm || conf.emit_mir) {
    {
      reporter.time("out of ssa");
      analysis::pass_manager pm = get_backend_pipeline();
      pm.after_pass = check_behaviour;
      for (auto& fn : ir_module.funcs)
        pm.run(fn, conf.verify_each);
    }
    if (conf.out_of_ssa)
      printer.print_ssa_ir(std::cout, ir_module);
  }

  if (conf.emit_asm || conf.emit_mir) {

    reporter.time("code generation");
    const auto entry = ir::find_entry(ir_module, sa.semantics);
    if (!entry)
      throw std::runtime_error("could not find 'main' entry point");
    mir::module machine = backend::select_instructions(
        ir_module, ir_module.funcs[*entry].return_type, static_cast<uint32_t>(*entry));
    if (conf.emit_mir)
      for (const auto& fn : machine.funcs)
        std::cout << mir::to_string(fn) << '\n';

    for (auto& fn : machine.funcs)
      backend::allocate_spill_all(fn);

    if (conf.emit_asm) {
      std::ofstream file(conf.out_name);
      if (!file)
        throw std::runtime_error("cannot write " + conf.out_name);
      backend::emit_module(file, machine);
    }
  }
}
}
