#pragma once

#include <memory>
#include <sstream>

namespace dungeon::diag {
    // NOTE TO SELF: some useful colors
    // std::cerr << "\033[1;32mSuccess!\033[m\n";
    // std::cerr << "\033[1;31mError!\033[m\n";
    // std::cerr << "\033[1;33mWarning!\033[m\n";
    template<typename... Args>
    void error(Args... args) {
        std::stringstream buf{};
        ( ((buf << " ") << args), ... );
        throw std::runtime_error(buf.str());
    }

    struct source_file {
        std::string name, data;

        source_file(std::string name, std::string data) : name{std::move(name)},
                                                          data{std::move(data)} {
        }
    };

    using source_ptr = std::shared_ptr<source_file>;

    struct src_location {
        source_ptr doc;
        int line = 1, col = 1, byte = 0;
    };

    inline std::ostream &operator<<(std::ostream &os, const src_location &loc) {
        os << "Ln " << loc.line << ", Col " << loc.col;
        return os;
    }

    enum class severity {
        warning,
        error,
    };

    // TODO: we actually would like to keep a source span instead of a src_location here
    struct diagnostic {
        severity level;
        src_location loc;
        std::string message;
    };
}
