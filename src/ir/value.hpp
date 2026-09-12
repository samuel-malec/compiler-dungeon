#pragma once
#include <cstdint>
#include <utility>

#include "../sema/types.hpp"

namespace dungeon::ir {
    struct instruction;

    struct value {
        uint32_t id;
        uint32_t version;
        const type *ty;
        std::vector<instruction *> users;
        instruction *defining_instruction = nullptr;

        value(uint32_t id, uint32_t version, const type *ty, std::vector<instruction *> users) : id(id),
            version{version}, ty(ty),
            users(std::move(users)) {
        }

        value(uint32_t id, const type *ty, std::vector<instruction *> users) : id(id), version{1}, ty(ty),
                                                                               users(std::move(users)) {
        }

        struct const_int {
            uint64_t value;
        };

        struct const_bool {
            bool value;
        };
    };
}
