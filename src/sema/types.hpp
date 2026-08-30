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
        type int8_{.kind = type_kind::_int, .bits = 8};
        type int16_{.kind = type_kind::_int, .bits = 16};
        type int32_{.kind = type_kind::_int, .bits = 32};
        type int64_{.kind = type_kind::_int, .bits = 64};

        type uint8_{.kind = type_kind::_uint, .bits = 8};
        type uint16_{.kind = type_kind::_uint, .bits = 16};
        type uint32_{.kind = type_kind::_uint, .bits = 32};
        type uint64_{.kind = type_kind::_uint, .bits = 64};

        type bool_{.kind = type_kind::_bool, .bits = 8};
        type unit_{.kind = type_kind::_unit, .bits = 8};

        const type *get_int(size_t bits) const {
            if (bits == 8) return &int8_;
            if (bits == 16) return &int16_;
            if (bits == 32) return &int32_;
            if (bits == 64) return &int64_;
            assert(false && "invalid type size");
        }

        const type *get_uint(size_t bits) const {
            if (bits == 8) return &uint8_;
            if (bits == 16) return &uint16_;
            if (bits == 32) return &uint32_;
            if (bits == 64) return &uint64_;
            assert(false && "invalid type size");
        }

        const type *get_bool() const {
            return &bool_;
        }

        const type *get_unit() const {
            return &unit_;
        }
    };

    // TODO: we actually need to refactor this, since this is retarded af
    inline bool same_type(const type *a, const type *b) {
        return a == b;
    }

    inline bool compatible_types(const type *a, const type *b) {
        return a == b;
    }

    inline bool is_boolean(const type *ty) {
        return ty->kind == type_kind::_bool;
    }

    inline bool is_signed_integer(const type *ty) {
        return ty->kind == type_kind::_int;
    }

    inline bool is_unsigned_integer(const type *ty) {
        return ty->kind == type_kind::_uint;
    }

    inline bool is_integer(const type *ty) {
        return is_signed_integer(ty) ||
               is_unsigned_integer(ty);
    }

    inline bool is_unit(const type *ty) {
        return ty->kind == type_kind::_unit;
    }

    inline const type *infer_unary(op_kind op, const type *lhs, type_manager &types) {
        if (!is_unary_op(op))
            diag::error("Invalid unary operation");
        return lhs;
    }

    inline const type *infer_relational(op_kind op, const type *lhs, const type *rhs, type_manager &types) {
        if (!is_rel_op(op))
            diag::error("Invalid relational operation");

        // TODO: some relational operations like ==, != make sense on bools, but does it make sense to compare bools ?
        // think about what operations are ok here;
        if (lhs == rhs)
            return types.get_bool();

        return nullptr;
    }

    inline const type *infer_numerical(op_kind op, const type *lhs, const type *rhs, type_manager &types) {
        if (!is_integer(lhs) || !is_integer(rhs))
            diag::error("Invalid numerical operation");

        if (lhs != rhs)
            return nullptr;

        return lhs;
    }

    inline const type *infer_binary(op_kind op, const type *lhs, const type *rhs, type_manager &types) {
        if (!is_binary_op(op))
            diag::error("Invalid binary operation");

        if (is_numerical_op(op))
            return infer_numerical(op, lhs, rhs, types);

        if (is_rel_op(op))
            return infer_relational(op, lhs, rhs, types);

        return nullptr;
    }
}
