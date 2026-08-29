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


    inline bool same_type(const type *a, const type *b) {
        return a == b;
    }

    inline bool compatible_types(const type *a, const type *b) {
        return a == b;
    }

    inline bool is_truthy_type(const type *a) {
        return a->kind == type_kind::_bool;
    }

    inline type *infer_binary(op_kind op, type *lhs, type *rhs) { return nullptr; }

    inline type *infer_unary(op_kind op, type *lhs) { return nullptr; }

    inline type *infer_relational(op_kind op, type *lhs, type *rhs) { return nullptr; }
}
