#pragma once
#include <cstdint>
#include <utility>

#include "../sema/types.hpp"

namespace dungeon::ir {
    struct instruction;

    struct value {
        uint32_t id;
        const type *ty;
        std::vector<instruction *> users;

        value(uint32_t id, const type *ty, std::vector<instruction *> users) : id(id), ty(ty),
            users(std::move(users)) {
        }
    };
}
