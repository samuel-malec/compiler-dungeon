#pragma once

#include <cassert>
#include <charconv>
#include <memory>
#include <string>
#include <set>
#include <vector>

#include "token.hpp"

namespace dungeon
{

static const std::set< std::string_view > keywords = {
    "let", "match", "static", "mut", "loop",
    "def", "if", "else", "for", "do", "while", "switch", "break",
    "continue", "case", "return", "assert", "struct", "enum",
    "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64",
    "unit", "bool", "true", "false",
};

static const std::set< std::string_view > punct = {
    "(", ")", "{", "}", "[", "]",
    ";", ",", ".", "?", ":", "+", "-", "*", "/", 
    "%", "|", "&", "^", "~", "=", "!", "<", ">",
    "++", "--", "&&", "||", "==", "!=", "<=", ">=",
    "<<", ">>", "->", "=>", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=",
};

struct lexer
{
    using sv_t = std::string_view;
    using cat = token::cat_t;

    location loc;
    sv_t sv;
    std::vector< token > toks;
    int ptr = 0;

    lexer( source_ptr doc ) : 
        loc{ doc },
        sv{ doc->data } {}

    std::vector< token > lex()
    {
        while ( !empty() )
        {
            next();
            advance();
        }

        return std::move( toks );
    }

    void next();

    bool empty() const { return sv.empty(); }

    void push( cat c ) { toks.emplace_back( token{ loc, sv.substr( 0, ptr ), c } ); }

    void advance( int offset = 0 )
    {
        assert( ptr + offset <= sv.size() );
        for ( auto c : sv.substr( 0, ptr + offset ) )
        {
            if ( c == '\n' )
            {
                loc.line++;
                loc.col = 1;
            }
            else
                loc.col++;
        }
        sv.remove_prefix( ptr );
        ptr = 0;
    }

    void drop_blanks()
    {
        while ( ptr < sv.size() && isspace( sv[ ptr ] ) )
            ++ptr;
        advance();
    }

    bool try_drop( sv_t str )
    {
        if ( sv.starts_with( str ) )
        {
            advance( str.size() );
            return true;
        }
        return false;
    }

    std::string_view shift_word() { return shift_word_with( '_' ); }

    std::string_view shift_word_with( auto... extra_allowed )
    {
        if ( sv.empty() || !( std::isalpha( static_cast<unsigned char>( sv.front() ) ) ||
            ( ( sv.front() == extra_allowed ) || ... ) ) )
            return {};

        while ( ptr < sv.size() &&
            ( std::isalnum( static_cast<unsigned char>( sv[ptr] ) ) ||
              ( ( sv[ptr] == extra_allowed ) || ... ) ) )
            ++ptr;
        return sv.substr( 0, ptr );
    }

    bool try_unsigned()
    {
        if ( sv.empty() )
            return false;

        if ( !std::isdigit( static_cast<unsigned char>( sv.front() ) ) )
            return false;

        while ( ptr < sv.size() && std::isdigit( static_cast<unsigned char>( sv[ptr] ) ) )
            ++ptr;
        if ( ptr + 1 < sv.size() && sv[ptr] == '.' &&
             std::isdigit( static_cast<unsigned char>( sv[ptr + 1] ) ) ) {
            ++ptr;
            while ( ptr < sv.size() && std::isdigit( static_cast<unsigned char>( sv[ptr] ) ) )
                ++ptr;
        }
        return true;
    }
};

}
