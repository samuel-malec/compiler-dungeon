#include "compiler.hpp"

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

        // Lexing
        lexer l{doc};
        std::vector<token> toks = l.lex();
        if (conf.emit_tokens || conf.stage == pipeline_stage::lexer)
            printer.print_tokens(toks);
        if (conf.stage == pipeline_stage::lexer)
            return;

        // Parsing
        parser p{std::move(toks)};
        auto ast = p.parse_module();
        if (!ast)
            throw std::runtime_error("parser failed");
        if (conf.emit_ast || conf.stage == pipeline_stage::parser)
            printer.print_ast_module(std::cout, ast.value());
        if (conf.stage == pipeline_stage::parser)
            return;

        // Semantic analysis
        sema::semantic_analyzer sa{};
        sa.run( ast.value() );
        if (conf.stage == pipeline_stage::semantic || conf.stage == pipeline_stage::typecheck)
            return;

        hir::module hir = hir::lower_ast_to_hir( ast.value(), sa.semantics );
        if ( conf.emit_hir || conf.stage == pipeline_stage::hir )
            printer.print_hir_module( std::cout, hir);
        if (conf.stage == pipeline_stage::hir)
            return;

        ir::module ir_module = ir::lower_hir_to_ir( hir, sa.semantics );
        if (conf.emit_ir || conf.stage == pipeline_stage::ir )
            printer.print_ir_module( std::cout, ir_module );
        if ( conf.stage == pipeline_stage::ir)
            return;

        ir::cfg_builder builder{};
        builder.build(ir_module);
        if ( conf.emit_cfg || conf.stage == pipeline_stage::cfg ) {}


        // TODO: should the analysis pipeline just be inlined here ? 
        // dungeon::analysis anal{};
        // anal.run_pipeline();
    }
}
