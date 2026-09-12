include<emscripten / bind.h>
#include <sstream>
#include <string>

#include "../analysis/constant_folding.hpp"
#include "../analysis/dce.hpp"
#include "../analysis/mem2reg.hpp"
#include "../analysis/pass_manager.hpp"
#include "../analysis/pipeline.hpp"
#include "../common/pretty_printer.hpp"
#include "../frontend/ast.hpp"
#include "../frontend/lexer.hpp"
#include "../frontend/parser.hpp"
#include "../frontend/token.hpp"
#include "../hir/ast2hir.hpp"
#include "../ir/cfg_builder.hpp"
#include "../ir/hir2ir.hpp"
#include "../sema/semantic.hpp"

namespace dungeon {
    struct compile_result {
        std::string tokens, ast, hir, ir, cfg;
        std::string stage;
        std::string error;
    };

    compile_result compile(const std::string &source) {
        compile_result r;
        print::pretty_printer printer{};
        try {
            diag::source_ptr doc = std::make_shared<diag::source_file>("<input>", source);

            // Lexing
            std::vector<token> toks;
            {
                lexer l{doc};
                toks = l.lex();
            }
            {
                std::ostringstream oss;
                printer.print_tokens(oss, toks);
                r.tokens = oss.str();
            }
            r.stage = "lexer";

            // Parsing
            parser p{std::move(toks)};
            std::optional<ast::module> ast_mod = p.parse_module();
            if (!ast_mod)
                throw std::runtime_error("parser failed");
            {
                std::ostringstream oss;
                printer.print_ast_module(oss, ast_mod.value());
                r.ast = oss.str();
            }
            r.stage = "parser";

            // Semantic analysis
            sema::semantic_analyzer sa{};
            sa.run(ast_mod.value());
            r.stage = "semantic";

            // AST -> HIR lowering
            hir::module hir_mod = hir::lower_ast_to_hir(ast_mod.value(), sa.semantics);
            {
                std::ostringstream oss;
                printer.print_hir_module(oss, hir_mod);
                r.hir = oss.str();
            }
            r.stage = "hir";

            // HIR -> IR lowering
            ir::module ir_mod = ir::lower_hir_to_ir(hir_mod, sa.semantics);
            {
                std::ostringstream oss;
                printer.print_ir_module(oss, ir_mod);
                r.ir = oss.str();
            }
            r.stage = "ir";

            // Building CFG + running the default analysis pipeline
            {
                ir::cfg_builder builder{};
                builder.build(ir_mod);

                analysis::pass_manager pm = get_default_pipeline();
                for (auto &fn: ir_mod.funcs)
                    pm.run(fn);
            }
            {
                std::ostringstream oss;
                printer.print_cfg_module(oss, ir_mod);
                r.cfg = oss.str();
            }
            r.stage = "analysis";

            r.stage = "done";
        } catch (const std::exception &e) {
            r.error = e.what();
        }
        return r;
    }

    struct optimize_result {
        std::string before;
        std::string after;
        std::string stage;
        std::string error;
    };

    optimize_result optimize(const std::string &source, bool enable_constant_folding, bool enable_dce) {
        optimize_result r;
        print::pretty_printer printer{};
        try {
            diag::source_ptr doc = std::make_shared<diag::source_file>("<input>", source);

            lexer l{doc};
            std::vector<token> toks = l.lex();
            r.stage = "lexer";

            parser p{std::move(toks)};
            std::optional<ast::module> ast_mod = p.parse_module();
            if (!ast_mod)
                throw std::runtime_error("parser failed");
            r.stage = "parser";

            sema::semantic_analyzer sa{};
            sa.run(ast_mod.value());
            r.stage = "semantic";

            hir::module hir_mod = hir::lower_ast_to_hir(ast_mod.value(), sa.semantics);
            r.stage = "hir";

            ir::module ir_mod = ir::lower_hir_to_ir(hir_mod, sa.semantics);
            r.stage = "ir";

            ir::cfg_builder builder{};
            builder.build(ir_mod);

            mem2reg m2r{};
            for (auto &fn: ir_mod.funcs)
                m2r.run(fn);
            {
                std::ostringstream oss;
                printer.print_cfg_module(oss, ir_mod);
                r.before = oss.str();
            }

            if (enable_constant_folding) {
                constant_folding cf{};
                for (auto &fn: ir_mod.funcs)
                    cf.run(fn);
            }
            if (enable_dce) {
                dce d{};
                for (auto &fn: ir_mod.funcs)
                    d.run(fn);
            }
            {
                std::ostringstream oss;
                printer.print_cfg_module(oss, ir_mod);
                r.after = oss.str();
            }
            r.stage = "done";
        } catch (const std::exception &e) {
            r.error = e.what();
        }
        return r;
    }
}

EMSCRIPTEN_BINDINGS (compiler_dungeon) {
    emscripten::value_object<dungeon::compile_result>("CompileResult")
            .field("tokens", &dungeon::compile_result::tokens)
            .field("ast", &dungeon::compile_result::ast)
            .field("hir", &dungeon::compile_result::hir)
            .field("ir", &dungeon::compile_result::ir)
            .field("cfg", &dungeon::compile_result::cfg)
            .field("stage", &dungeon::compile_result::stage)
            .field("error", &dungeon::compile_result::error);
    emscripten::function("compile", &dungeon::compile);

    emscripten::value_object<dungeon::optimize_result>("OptimizeResult")
            .field("before", &dungeon::optimize_result::before)
            .field("after", &dungeon::optimize_result::after)
            .field("stage", &dungeon::optimize_result::stage)
            .field("error", &dungeon::optimize_result::error);
    emscripten::function("optimize", &dungeon::optimize);
}
