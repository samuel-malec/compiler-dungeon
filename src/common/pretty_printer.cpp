#include <optional>

#include "pretty_printer.hpp"
#include "../frontend/ast.hpp"

namespace dungeon::print
{
using expr = ast::expr;
using stmt = ast::stmt;
using toplevel = ast::toplevel;
using var_decl = ast::let_data;
using fn_decl = ast::fn_decl;
using enum_decl = ast::enum_decl;
using struct_decl = ast::struct_decl; 
using atom_map = std::map< uint32_t, std::string >;

// TODO: complete the printing
void pretty_printer::print_expr( expr& e, int depth )
{
    // if ( auto t = std::get_if< ast::num_lit_data >( e.data ) )
    // {}
    // if ( auto t = std::get_if< ast::bool_lit_data >( e.data ) )
    // {}
    // if ( auto t = std::get_if< ast::identifier_data >( e.data ) )
    // {}
    // if ( auto t = std::get_if< ast::unary_data >( e.data ) )
    // {}
    // if ( auto t = std::get_if< ast::binary_data >( e.data ) )
    // {}
    // if ( auto t = std::get_if< ast::relational_data >( e.data ) )
    // {}
    // if ( auto t = std::get_if< ast::assign_data >( e.data ) )
    // {}
    // if ( auto t = std::get_if< ast::call_data >( e.data ) )
    // {}
}

// TODO: complete the printing
void pretty_printer::print_stmt( stmt& s, int depth )
{
    // if ( auto t = std::get_if< ast::ret_data >( s.data ) ) {}
    // if ( auto t = std::get_if< ast::if_data >( s.data ) ) {}
    // if ( auto t = std::get_if< ast::for_data >( s.data ) ) {}
    // if ( auto t = std::get_if< ast::while_data >( s.data ) ) {}
    // if ( auto t = std::get_if< ast::do_while_data >( s.data ) ) {}
    // if ( auto t = std::get_if< ast::cont_data >( s.data ) ) {}
    // if ( auto t = std::get_if< ast::brk_data >( s.data ) ) {}
    // if ( auto t = std::get_if< ast::block_data >( s.data ) ) {}
    // if ( auto t = std::get_if< ast::var_decl >( s.data ) ) {}
    // if ( auto t = std::get_if< ast::expr_stmt_data >( s.data ) ) {}
}

void pretty_printer::print_ast_module(ast::module& ast_module)
{

    // for ( auto& decl : ast_module.toplevel_items )
    // {
    //     std::visit( [ this ]( auto&& arg )
    //     {
    //         using T = std::decay_t< decltype( arg ) >;
    //         if constexpr ( std::is_same_v< T, fn_decl > )
    //         {
    //             std::cout << "fn_decl: ";
    //             fn_decl fn = arg;
    //             std::cout << fn.sig.ret_type << " " << fn.name << "( ";
    //             for ( size_t i = 0; i < fn.params.size(); ++i )
    //             {
    //                 auto& p = fn.params[ i ];
    //                 // std::cout << p.typ << " " << p.name << ( i == fn.params.size() - 1 ?  "" : ", " );
    //             }
    //
    //             std::cout << " )\n";
    //             for ( auto& s : fn.body )
    //                 print_stmt( s, 1 );
    //         }
    //         else if constexpr ( std::is_same_v< T, struct_decl > )
    //             std::cout << "struct_decl\n";
    //         else if constexpr ( std::is_same_v< T, enum_decl > )
    //             std::cout << "enum_decl\n";
    //         else
    //             static_assert( false, "non-exhaustive visitor!" );
    //     }, decl );
    // }
}

void pretty_printer::print_hir_expr( hir::expr& e, int depth, const atom_map& am )
{
}

void pretty_printer::print_hir_stmt( hir::stmt& s, int depth, const atom_map& am )
{
}

void pretty_printer::print_hir( hir::program& hir, const atom_map& am )
{
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
