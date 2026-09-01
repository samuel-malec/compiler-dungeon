#include "compiler.hpp"

#include "../common/file.hpp"
#include "../common/pretty_printer.hpp"

#include "../frontend/parser.hpp"
#include "../sema/semantic.hpp"
#include "../frontend/token.hpp"

#include "../hir/ast2hir.hpp"
#include "../ir/cfg/cfg.hpp"

namespace dungeon {
    void compiler::run(config &conf) {
        std::string in_name = conf.in_name;
        std::string out_name = conf.out_name;
        source_ptr doc = std::make_shared<source_file>(in_name, read_file(in_name));
        print::pretty_printer printer{};

        // Lexing
        lexer l{doc};
        std::vector<token> toks = l.lex();
        if (conf.emit_tokens)
            printer.print_tokens(toks);

        // Parsing
        parser p{std::move(toks)};
        auto ast = p.parse_module();
        if (conf.emit_ast)
            printer.print_ast_module(std::cout, ast.value());

        // Semantic analysis
        sema::semantic_analyzer sa{};
        sa.run( ast.value() );

        hir::module hir = hir::lower_ast_to_hir( ast.value(), sa.semantics );
        if ( conf.emit_hir )
            printer.print_hir_module( hir );

        // tac::program tac_ir = tac::lower_to_tac( hir );
        // if ( conf.emit_tac )
        //     printer.print_tac( tac_ir, sa.st.reverse_map );

        // cfg::program cfgraph = cfg::build_cfg( tac_ir );


        // TODO: should the analysis pipeline just be inlined here ? 
        // dungeon::analysis anal{};
        // anal.run_pipeline();
    }
}
