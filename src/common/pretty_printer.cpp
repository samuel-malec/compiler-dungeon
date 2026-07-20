#include "pretty_printer.hpp"

namespace dungeon::print
{
// TODO: so far we have been using std::cout, but this is no longer the logical thing to do since we are 
// past the stage of testing small programs, and instead we should focus on dumping the intermediate representations into a file
// basically, we should remove the std::cout, pass explicit std::ostream& os argument into all of these functions, seems like pain in the ass
using expr = ast::expr;
using stmt = ast::stmt;
using toplevel = ast::toplevel;
using var_decl = ast::var_decl;
using fn_decl = ast::fn_decl;
using enum_decl = ast::enum_decl;
using struct_decl = ast::struct_decl; 
using atom_map = std::map< uint32_t, std::string >;

void pretty_printer::print_expr( expr& e, int depth )
{
    pad( depth );
    std::cout << e.typ << " ";
    switch ( e.cat )
    {
        case expr::num_lit:
            std::cout << "[ num_lit ] " << std::get< uint64_t >( e.val );
            break;
        case expr::bool_lit:
            std::cout << "[ bool_lit ] " << std::get< bool >( e.val );
            break;
        case expr::identifier:
            std::cout << "[ id ] " << e.id;
            break;
        case expr::unary:
            std::cout << "[ unary " << e.op << " ]\n";
            print_expr( e[ 0 ], depth + 1 );
            return;
        case expr::binary:
            std::cout << "[ binary " << e.op << " ]\n";
            print_expr( e[ 0 ], depth + 1 );
            print_expr( e[ 1 ], depth + 1 );
            return;
        case expr::relational:
            std::cout << "[ rel " << e.op << " ]\n";
            print_expr( e[ 0 ], depth + 1 );
            print_expr( e[ 1 ], depth + 1 );
            return;
        case expr::assign:
            std::cout << "[ assign (" << e.op << ") ]\n";
            print_expr( e[ 0 ], depth + 1 );
            print_expr( e[ 1 ], depth + 1 );
            return;
        case expr::call:
            std::cout << "[ call ]\n";
            print_expr( e.subs[ 0 ], depth + 1 );
            pad( depth + 1 );
            std::cout << "[ params ]\n";
            for ( int i = 1; i < e.subs.size(); ++ i )
                print_expr( e.subs[ i ], depth + 2 );
            return;

        default:
            break;
    }
    std::cout << '\n';
}

void pretty_printer::print_stmt( stmt& s, int depth )
{
    pad( depth );
    switch ( s.cat )
    {
        case stmt::ret:
            std::cout << "[ ret ]";
            if  ( s.e.has_value() )
            {
                std::cout << "\n";
                print_expr( s.e.value(), depth + 1 );
            }
            else
                std::cout << " {}\n";
            return;
        
        case stmt::if_stmt:
            std::cout << "[ if_stmt ]\n";
            pad( depth + 1 );
            std::cout << "[ cond ]\n";
            if  ( s.e.has_value() )
                print_expr( s.e.value(), depth + 2 );
            else
                std::cout << "{}";
            
            print_stmt( s[ 0 ], depth + 1 );
            if ( s.subs.size() > 1 )
            {
                pad( depth );
                std::cout << "[ else ]\n";
                print_stmt( s[ 1 ], depth + 1 );
            }
            return;
        
        case stmt::for_stmt:
            std::cout << "[ for_stmt ]\n";
            for ( auto& se : s.subs )
                print_stmt( se, depth + 1 );
            return;

        case stmt::while_stmt:
            std::cout << "[ while_stmt ]\n";
            pad( depth );
            std::cout << "[ cond ]\n";
            if ( !s.e.has_value() )
                std::cout << " {} ";
            else
                print_expr( s.e.value(), depth );

            std::cout << "\n";
            print_stmt( s[ 0 ], depth + 1 );
            return;

        case stmt::do_while_stmt:
            std::cout << " [ do_while ]\n";
            print_stmt( s[ 0 ], depth + 1 );
            pad( depth );
            std::cout << "[ cond ]\n";
            if ( !s.e.has_value() )
                std::cout << " {} ";
            else
                print_expr( s.e.value(), depth );
            return;

        case stmt::cont:
            std::cout << "[ continue ]";
            break;

        case stmt::brk:
            std::cout << "[ break ]";
            break;

        case stmt::block:
            std::cout << "[ block ]\n";
            for ( auto& se : s.subs )
                print_stmt( se, depth + 1 );
            return;

        case stmt::var_dclr:
            std::cout << "[ var_decl ]\n";
            pad( depth + 1 );
            std::cout << "[ var ] " << s.vdecl.typ << " " << s.vdecl.name << '\n';
            if ( s.vdecl.e.has_value() )
                print_expr( s.vdecl.e.value(), depth + 1 );
            return;

        case stmt::expr_stmt:
            std::cout << "[ expr_stmt ]\n";
            if ( !s.e.has_value() )
                std::cout << " {} ";
            else
                print_expr( s.e.value(), depth + 1 );
            return;

        default:
            break;
    }

    std::cout << "\n";
}

void pretty_printer::print_ast( ast::program& ast )
{
    for ( auto& decl : ast.toplevel_items )
    {
        std::visit( [ this ]( auto&& arg )
        {
            using T = std::decay_t< decltype( arg ) >;
            if constexpr ( std::is_same_v< T, fn_decl > )
            {
                std::cout << "fn_decl: ";
                fn_decl fn = arg;
                std::cout << fn.sig.ret_type << " " << fn.name << "( ";
                for ( size_t i = 0; i < fn.params.size(); ++i )
                {
                    auto& p = fn.params[ i ];
                    std::cout << p.typ << " " << p.name << ( i == fn.params.size() - 1 ?  "" : ", " );
                }
                
                std::cout << " )\n";
                for ( auto& s : fn.body )
                    print_stmt( s, 1 );
            }
            else if constexpr ( std::is_same_v< T, struct_decl > )
                std::cout << "struct_decl\n";
            else if constexpr ( std::is_same_v< T, enum_decl > )
                std::cout << "enum_decl\n";
            else
                static_assert( false, "non-exhaustive visitor!" );
        }, decl );
    }
}

void pretty_printer::print_hir_expr( hir::expr& e, int depth, const atom_map& am )
{
    pad( depth );
    std::cout << "[hir:" << e.typ << "] ";
    if ( std::holds_alternative< uint64_t >( e.data ) )
        std::cout << "[int_lit] " << std::get< uint64_t >( e.data ) << "\n";
    else if ( std::holds_alternative< bool >( e.data ) ) 
        std::cout << "[bool_lit] " << ( std::get< bool >( e.data ) ? "true" : "false" ) << "\n";
    
    else if ( std::holds_alternative< hir::expr::var_ref_data >( e.data ) ) 
    {
        auto data = std::get< hir::expr::var_ref_data >( e.data );
        std::cout << "[var_ref] " << am.at( data.id ) << "\n";
    }

    else if ( std::holds_alternative< hir::expr::unary_data >( e.data ) ) 
    {
        auto data = std::get< hir::expr::unary_data >( e.data );
        std::cout << "[unary " << data.op << "]\n";
        if ( data.sub ) print_hir_expr( *data.sub, depth + 1, am );
    }

    else if ( std::holds_alternative< hir::expr::binary_data >( e.data ) ) 
    {
        auto data = std::get< hir::expr::binary_data >( e.data );
        std::cout << "[binary " << data.op << "]\n";
        if ( data.left )  print_hir_expr( *data.left, depth + 1, am );
        if ( data.right ) print_hir_expr( *data.right, depth + 1, am );
    }

    else if ( std::holds_alternative< hir::expr::assign_data >( e.data ) ) 
    {
        auto data = std::get< hir::expr::assign_data >( e.data );
        std::cout << "[assign] v" << data.target << "\n";
        if ( data.value ) print_hir_expr( *data.value, depth + 1, am );
    }

    else if ( std::holds_alternative< hir::expr::call_data >( e.data ) ) 
    {
        auto data = std::get< hir::expr::call_data >( e.data );
        std::cout << "[call] f" << data.target << "\n";
        pad( depth + 1 );
        std::cout << "[args]\n";
        for ( const auto& arg : data.args )
            if ( arg ) print_hir_expr( *arg, depth + 2, am );
    }
    // case hir::expr::cast:
    //     std::cout << "[cast]\n";
    //     break;
    // }
}

void pretty_printer::print_hir_stmt( hir::stmt& s, int depth, const atom_map& am )
{
    pad( depth );
    if ( std::holds_alternative< hir::stmt::expr_data >( s.data ) )
    {
        std::cout << "[expr_stmt]\n";
        print_hir_expr( std::get< hir::stmt::expr_data >( s.data ).e, depth + 1, am );
    }
    else if ( std::holds_alternative< hir::stmt::block_data >( s.data ) )
    {
        std::cout << "[block]\n";
        auto data = std::get< hir::stmt::block_data >( s.data );
        for ( auto& sub_stmt : data.stmts )
            print_hir_stmt( sub_stmt, depth + 1, am );
    }

    else if ( std::holds_alternative< hir::stmt::let_data >( s.data ) )
    {
        auto data = std::get< hir::stmt::let_data >( s.data );
        std::cout << "[let_stmt] " << data.typ << " " << am.at( data.target ) << "\n";
        if ( data.value ) 
            print_hir_expr( *data.value, depth + 1, am );
    }

    else if ( std::holds_alternative< hir::stmt::if_data >( s.data ) )
    {
        auto data = std::get< hir::stmt::if_data >( s.data );
        std::cout << "[if_stmt]\n";
        pad( depth + 1 );
        std::cout << "[cond]\n";
        print_hir_expr( data.cond, depth + 2, am );
        
        if ( data.then_branch )
        {
            pad( depth + 1 );
            std::cout << "[then]\n";
            print_hir_stmt( *data.then_branch, depth + 2, am );
        }
        if ( data.else_branch )
        {
            pad( depth + 1 );
            std::cout << "[else]\n";
            print_hir_stmt( *data.else_branch, depth + 2, am );
        }
    }

    else if ( std::holds_alternative< hir::stmt::loop_data >( s.data ) )
    {
        auto data = std::get< hir::stmt::loop_data >( s.data );
        std::cout << "[loop_stmt]\n";
        if ( data.body ) 
            print_hir_stmt( *data.body, depth + 1, am );
    }

    else if ( std::holds_alternative< hir::stmt::brk >( s.data ) )
        std::cout << "[break]\n";

    else if ( std::holds_alternative< hir::stmt::cont >( s.data ) )
        std::cout << "[continue]\n";

    else if ( std::holds_alternative< hir::stmt::ret_data >( s.data ) )
    {
        auto data = std::get< hir::stmt::ret_data >( s.data );
        std::cout << "[ret]";
        if ( data.value )
        {
            std::cout << "\n";
            print_hir_expr( *data.value, depth + 1, am );
        }
        else
            std::cout << " {}\n";
    }
}

void pretty_printer::print_hir( hir::program& hir, const atom_map& am )
{
    if ( !hir.globals.empty() )
    {
        std::cout << "Globals:\n";
        for ( const auto& global : hir.globals )
        {
            pad( 1 );
            std::cout << "let v" << global.target << "\n";
            if ( global.value ) 
                print_hir_expr( *global.value, 2, am );
        }
    }

    for ( auto& fn : hir.functions )
    {
        std::cout << "fn_def: f" << fn.name << " :: " << fn.sig << " params: ( ";
        for ( size_t i = 0; i < fn.params.size(); ++i )
        {
            std::cout << "v" << fn.params[ i ] << ( i == fn.params.size() - 1 ? "" : ", " );
        }
        std::cout << " )\n";
        print_hir_stmt( fn.body, 1, am );
    }
}

std::string pretty_printer::tac_operand_to_string( tac::operand& operand, const atom_map& am )
{
    return std::visit( [ this, am ]( auto&& value ) -> std::string
    {
        using T = std::decay_t< decltype( value ) >;
        if constexpr ( std::is_same_v< T, tac::value > )
            return tac_val_to_string( value, am );
        else
        {
            std::ostringstream out;
            if ( std::holds_alternative< uint64_t >( value ) )
                out << std::get< uint64_t >( value );
            if ( std::holds_alternative< bool >( value ) )
                out << ( std::get< bool >( value ) ? "true" : "false" );
            return out.str();
        }
    }, operand );
}

std::string pretty_printer::tac_instr_symbolic( tac::instr& i, const atom_map& am )
{
    return std::visit( [ this, am ]( auto&& value ) -> std::string
    {
        using T = std::decay_t< decltype( value ) >;
        std::ostringstream out;

        if constexpr ( std::is_same_v< T, tac::unary_data > )
        {
            out << tac_val_to_string( value.target, am ) << " ← "
                << value.op << " "
                << tac_operand_to_string( value.arg1, am );
        }
        else if constexpr ( std::is_same_v< T, tac::binary_data > )
        {
            out << tac_val_to_string( value.target, am ) << " ← "
                << tac_operand_to_string( value.arg1, am ) << " "
                << value.op << " "
                << tac_operand_to_string( value.arg2, am );
        }
        else if constexpr ( std::is_same_v< T, tac::copy_data > )
        {
            out << tac_val_to_string( value.target, am ) << " ← "
                << tac_operand_to_string( value.arg1, am );
        }
        else if constexpr ( std::is_same_v< T, tac::jump_data > )
        {
            out << "jump ";
            out << value.label;
        }
        else if constexpr ( std::is_same_v< T, tac::branch_data > )
        {
            out << "branch " << tac_operand_to_string( value.arg1, am )
                << "  " << value.true_lab << " " << value.false_lab;
        }
        else if constexpr ( std::is_same_v< T, tac::param_data > )
        {
            out << "param ";
            out << tac_operand_to_string( value.arg, am );
        }
        else if constexpr ( std::is_same_v< T, tac::get_param_data> )
        {
            out << tac_val_to_string( value.target, am ) << " ← get_param "
            << value.idx;
        }
        else if constexpr ( std::is_same_v< T, tac::call_data > )
        {
            out << tac_val_to_string( value.target, am ) << " ← call "
                << value.callee << "(" << value.args << " args)";
        }
        else if constexpr ( std::is_same_v< T, tac::label_data > )
        {
            out << value.id << ":";
        }
        else if constexpr ( std::is_same_v< T, tac::ret_data > )
        {
            out << "ret";
            if ( value.arg.has_value() )
                out << " " << tac_operand_to_string( value.arg.value(), am );
        }

        return out.str();
    }, i.data );
}


void pretty_printer::print_tac( tac::program& tac, const atom_map& am  )
{
    for ( auto& fn : tac.functions )
    {
        std::cout << "f" << fn.name << " :: ";
        std::cout << fn.sig;
        for ( auto& i : fn.body )
            print_tac_inst( i, am );
    }
}

std::string value_to_string( const tac::value& v, const atom_map& am )
{
    std::ostringstream os;
    os << "v" << v.id << "." << v.version;
    return os.str();
}

std::string phi_to_string( const cfg::phi_node& phi, const atom_map& am )
{
    std::ostringstream os;
    os << value_to_string( phi.res, am ) << " = φ(";
    bool first = true;
    for ( auto& [ pred_id, val ] : phi.incoming )
    {
        if ( !first ) os << ", ";
        os << "bb" << pred_id << ": " << value_to_string( val, am );
        first = false;
    }
    os << ")";
    return os.str();
}

void pretty_printer::export_to_dot( cfg::cfg& graph, std::ostream& out, const atom_map& am )
{
    out << "digraph CFG {\n";
    out << "    node [shape=box, fontname=\"Courier New\", fontsize=10, style=filled, fillcolor=\"#f9f9f9\"];\n";
    out << "    edge [fontname=\"Courier New\", fontsize=9];\n\n";

    if ( !graph.basic_blocks.empty() )
    {
        out << "    entry [shape=circle, label=\"entry\", style=filled, fillcolor=\"#d4edda\", fontname=\"Courier New\", fontsize=10, width=0.5, fixedsize=true];\n";
        out << "    entry -> block_" << graph.basic_blocks.front()->id << ";\n\n";
    }

    for ( const auto& bb : graph.basic_blocks )
    {
        out << "    block_" << bb->id << " [label=\"";
        out << "BB " << bb->id << "\\n";
        out << "--------------------------------\\n";
        
        if ( !bb->phis.empty() )
        {
            for ( auto& phi : bb->phis )
            {
                std::string s = phi_to_string( phi, am );
                size_t pos = 0;
                while ( ( pos = s.find( '"', pos ) ) != std::string::npos )
                {
                    s.replace( pos, 1, "\\\"" );
                    pos += 2;
                }
                out << s << "\\n";
            }
            out << "................................\\n";
        }

        for ( auto& ins : bb->instructions )
        {
            std::string inst_str = "";
            
            if ( std::holds_alternative< tac::branch_data >( ins.data ) )
            {
                std::ostringstream os;
                os << "branch ";
                auto bd = std::get< tac::branch_data >( ins.data );
                os << tac_operand_to_string( bd.arg1, am );
                inst_str = os.str();
            }
            else if ( std::holds_alternative< tac::jump_data >( ins.data ) )
                inst_str = "jump";
            else
                inst_str = tac_instr_symbolic( ins, am );
            
            size_t pos = 0;
            while ( ( pos = inst_str.find( '"', pos ) ) != std::string::npos )
            {
                inst_str.replace( pos, 1, "\\\"" );
                pos += 2;
            }
            out << inst_str << "\\n";
        }
        out << "\"];\n";
    }

    out << "\n";

    for ( const auto& bb : graph.basic_blocks )
    {
        bool is_conditional = false;
        if ( !bb->instructions.empty() )
        {
            const auto& term = bb->instructions.back();
            if ( std::holds_alternative< tac::branch_data >( term.data ) )
            {
                is_conditional = true;
            }
        }

        for ( size_t i = 0; i < bb->succ.size(); ++i )
        {
            const auto succ = bb->succ[ i ];
            if ( succ )
            {
                out << "    block_" << bb->id << " -> block_" << succ->id;
                
                if ( is_conditional )
                {
                    if ( i == 0 )
                        out << " [label=\"true\", color=\"#2ca02c\", fontcolor=\"#2ca02c\"]"; // Forest Green
                    else if ( i == 1 )
                        out << " [label=\"false\", color=\"#d62728\", fontcolor=\"#d62728\"]"; // Crimson Red
                }
                out << ";\n";
            }
        }
    }

    out << "}\n";
}

};
