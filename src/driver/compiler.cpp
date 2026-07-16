#include "compiler.hpp"

#include "../common/file.hpp"
#include "../common/pretty_printer.hpp"

#include "../frontend/parser.hpp"
#include "../frontend/semantic.hpp"
#include "../frontend/token.hpp"

#include "../middleend/hir/ast2hir.hpp"
#include "../middleend/tac/hir2tac.hpp"

#include "../middleend/cfg/cfg.hpp"
#include "../middleend/cfg/ssa.hpp"

namespace dungeon
{
    void compiler::run( config& conf )
    {
        std::string in_name = conf.in_name;
        std::string out_name = conf.out_name;
        
        source_ptr doc = std::make_shared< source_file >( in_name, read_file( in_name ) ); 

        print::pretty_printer printer{};
        parser p{ doc };
        
        auto ast = p.parse();
        if ( conf.emit_ast )
            printer.print_ast( ast );

        semantic_analyzer sa{};
        sa.run( ast );
        
        hir::program hir = hir::lower_ast_to_hir( ast, sa.st );
        if ( conf.emit_hir )
            printer.print_hir( hir, sa.st.reverse_map );
        
        tac::program tac_ir = tac::lower_to_tac( hir );
        if ( conf.emit_tac )
            printer.print_tac( tac_ir, sa.st.reverse_map );

        cfg::program cfgraph = cfg::build_cfg( tac_ir );
        cfg::ssa_builder sb{};

        for ( auto& [ fn, cfg ] : cfgraph.fns )
        {
            sb.transform_ssa( cfg );
            if ( conf.emit_cfg )
            {
                std::ostringstream os;
                os << "f" << fn << "_cfg.dot";
                std::ofstream out( os.str() );
                printer.export_to_dot( cfg, out, sa.st.reverse_map );
            }
        }


    }
}
