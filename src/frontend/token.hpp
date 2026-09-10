#pragma once

#include <ostream>
#include <string>
#include <string_view>

#include "../diag/diag.hpp"

namespace dungeon {
    struct token {
        diag::src_location loc;
        std::string_view data;

        enum cat_t {
            invalid,
            punct,
            keyword,
            ident,
            number,
            string,
        } cat = invalid;
    };

    inline std::ostream &operator<<(std::ostream &os, const token::cat_t c) {
        switch (c) {
            case token::punct: return os << "punct";
            case token::keyword: return os << "keyword";
            case token::ident: return os << "ident";
            case token::number: return os << "number";
            case token::string: return os << "string";
            case token::invalid: return os << "invalid";
        }
        return os << "unknown";
    }

    inline std::ostream &operator<<(std::ostream &os, const token &t) {
        os << t.loc << "[ " << t.data << ", " << t.cat << " ]";
        return os;
    }
}
