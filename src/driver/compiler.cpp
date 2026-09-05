#include "compiler.hpp"

#include "progress_reporter.hpp"
#include "../common/file.hpp"
#include "../common/pretty_printer.hpp"

#include "../frontend/parser.hpp"
#include "../sema/semantic.hpp"
#include "../frontend/token.hpp"

#include "../hir/ast2hir.hpp"
#include "../ir/hir2ir.hpp"

namespace dungeon {
    void compiler::run(config &conf) {
        std::string in_name = conf.in_name;
        std::string out_name = conf.out_name;
        source_ptr doc = std::make_shared<source_file>(in_name, read_file(in_name));
        print::pretty_printer printer{};
        progress_reporter reporter{};

        // Lexing
        std::vector<token> toks;
        {
            reporter.time("lexing");
            lexer l{doc};
            toks = l.lex();
            if (conf.emit_tokens || conf.stage == pipeline_stage::lexer)
                printer.print_tokens(toks);
            if (conf.stage == pipeline_stage::lexer)
                return;
        }

        // Parsing
        parser p{std::move(toks)};
        std::optional<ast::module> ast;

        {
            reporter.time("parsing");
            ast = p.parse_module();
            if (!ast)
                throw std::runtime_error("parser failed");
            if (conf.emit_ast || conf.stage == pipeline_stage::parser)
                printer.print_ast_module(std::cout, ast.value());
            if (conf.stage == pipeline_stage::parser)
                return;
        }

        // Semantic analysis
        sema::semantic_analyzer sa{};
        {
            reporter.time("parsing");
            sa.run(ast.value());
            if (conf.stage == pipeline_stage::semantic || conf.stage == pipeline_stage::typecheck)
                return;
        }

        // AST -> HIR lowering
        hir::module hir;
        {
            reporter.time("hir lowering");
            hir = hir::lower_ast_to_hir(ast.value(), sa.semantics);
            if (conf.emit_hir || conf.stage == pipeline_stage::hir)
                printer.print_hir_module(std::cout, hir);
            if (conf.stage == pipeline_stage::hir)
                return;
        }

        // Generating IR
        ir::module ir_module;
        {
            reporter.time("generating ir");
            ir_module = ir::lower_hir_to_ir(hir, sa.semantics);
            if (conf.emit_ir || conf.stage == pipeline_stage::ir)
                printer.print_ir_module(std::cout, ir_module);
            if (conf.stage == pipeline_stage::ir)
                return;
        }

        // Building CFG
        {
            reporter.time("building cfg");
            ir::cfg_builder builder{};
            builder.build(ir_module);
        }

        if (conf.emit_cfg || conf.stage == pipeline_stage::cfg) {
            std::string file_name = "cfg.dot";
            std::ofstream ofs{file_name};
            if (!ofs)
                diag::error("Couldn't open file:", file_name);
            printer.export_to_dot(ofs, ir_module);
        }
    }
}
