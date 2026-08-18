#include <format>
#include <stdexcept>
#include <vector>

#include "lexer.hpp"

namespace dungeon
{

void lexer::next()
{
    drop_blanks();
    if ( empty() )
        return;
 
    if ( try_drop( "//" ) )
    {
        auto newline = sv.find( '\n', ptr );
        ptr = newline == sv.npos ? sv.size() : newline + 1;
        return;
    }

    if ( try_drop( "/*" ) )
    {
        auto end = sv.find( "*/", ptr );
        if ( end == sv.npos )
            throw std::runtime_error( "unterminated multi-line comment" );
        ptr = end + 2;
        return;
    }

    if ( sv.starts_with( '"' ) )
    {
        ptr = 1;
        bool escaped = false;
        while ( ptr < sv.size() ) {
            char c = sv[ptr++];
            if ( c == '"' && !escaped ) {
                push( cat::string );
                return;
            }
            escaped = c == '\\' && !escaped;
            if ( c != '\\' )
                escaped = false;
        }
        throw std::runtime_error( "unterminated string literal" );
    }

    // keyword
    auto word = shift_word();
    if ( keywords.contains( word ) )
    {
        push( cat::keyword );
        return;
    }

    // identifier
    if ( !word.empty() )
    {
        push( cat::ident );
        return;
    }

    // punctuation
    for ( int l : { 3, 2, 1 } )
    {
        if ( sv.size() < l )
            continue;
        
        auto s = sv.substr( 0, l );
        if ( punct.contains( s ) )
        {
            ptr += l;
            push( cat::punct );
            return;
        }
    }

    // number
    if ( try_unsigned() )
    {
        push( cat::number );
        return;
    }
    
    throw std::runtime_error( std::format("unexpected input at file: '{}', Ln {}, Col {}", 
                              loc.doc->name, loc.line, loc.col ) );
}

}
