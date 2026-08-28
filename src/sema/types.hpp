#pragma once

#include "../frontend/ast.hpp"

namespace dungeon {
    enum class type_kind {
        _int,
        _uint,
        _bool,
        _unit,
    };

    struct type {
        type_kind kind;
        size_t bits;
    };

    // TODO: how to make sure we cannot accidentally create two separately constructed int32 types ??
    struct type_manager {
        type int8_;
        type int16_;
        type int32_;
        type int64_;

        type uint8_;
        type uint16_;
        type uint32_;
        type uint64_;

        type bool_;
        type unit_;

        type *get_int(size_t bits) {
            if (bits == 8) return &int8_;
            if (bits == 16) return &int16_;
            if (bits == 32) return &int32_;
            if (bits == 64) return &int64_;
            assert(false && "invalid type size");
        }

        type *get_uint(size_t bits) {
            if (bits == 8) return &uint8_;
            if (bits == 16) return &uint16_;
            if (bits == 32) return &uint32_;
            if (bits == 64) return &uint64_;
            assert(false && "invalid type size");
        }

        type *get_bool() {
            return &bool_;
        }

        type *get_unit() {
            return &unit_;
        }
    };

    inline type *type_from_annotation(ast::type_annotation annotation) {
        type_manager tm{};
        if (annotation.base_name == "i8") {
            return tm.get_int(8);
        }
        if (annotation.base_name == "i16") {
            return tm.get_int(16);
        }
        if (annotation.base_name == "i32") {
            return tm.get_int(32);
        }
        if (annotation.base_name == "i64") {
            return tm.get_int(64);
        }
        if (annotation.base_name == "u8") {
            return tm.get_uint(8);
        }
        if (annotation.base_name == "u16") {
            return tm.get_uint(16);
        }
        if (annotation.base_name == "u32") {
            return tm.get_uint(32);
        }
        if (annotation.base_name == "u64") {
            return tm.get_uint(64);
        }
        if (annotation.base_name == "bool") {
            return tm.get_bool();
        }
        if (annotation.base_name == "unit") {
            return tm.get_unit();
        }
        assert(false && "unknown type");
    }

    inline bool same_type(const type *a, const type *b) {
        return a == b;
    }
}
