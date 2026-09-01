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

    // TODO: think about what are the prerequisities of thiese infer_** should we check the ops kinds are good or is this a precondition ?
    // TODO: actually think about how to infer these types, this is a preliminiary impl
    inline const type *infer_unary(op_kind op, const type *lhs, type_manager &types) {
        if (!is_unary_op(op))
            diag::error("Invalid unary operation");
        return lhs;
    }

    inline const type *infer_equality_op(op_kind op, const type *lhs, const type *rhs, const type_manager &types) {
        if (lhs != rhs)
            diag::error("Expected equal types");
        return types.get_bool();
    }

    inline const type *infer_ordering_op(op_kind op, const type *lhs, const type *rhs, const type_manager &types) {
        if (lhs != rhs)
            diag::error("Expected ordering types");

        return types.get_bool();
    }

    inline const type *infer_relational(op_kind op, const type *lhs, const type *rhs, type_manager &types) {
        if (!is_rel_op(op))
            diag::error("Invalid relational operation");

        if (is_equality_op(op))
            return infer_equality_op(op, lhs, rhs, types);

        if (is_ordering_op(op))
            return infer_ordering_op(op, lhs, rhs, types);

        return nullptr;
    }

    inline const type *infer_numerical(op_kind op, const type *lhs, const type *rhs, type_manager &types) {
        if (!is_integer(lhs) || !is_integer(rhs))
            diag::error("Invalid numerical operation");

        if (lhs != rhs)
            return nullptr;

        return lhs;
    }

    inline const type *infer_logical_op(op_kind op, const type *lhs, const type *rhs, const type_manager &types) {
        if (!is_boolean(lhs) || !is_boolean(rhs))
            diag::error("Invalid operands, expected booleans");

        return lhs;
    }

    inline const type *infer_binary(op_kind op, const type *lhs, const type *rhs, type_manager &types) {
        if (!is_binary_op(op))
            diag::error("Invalid binary operation");

        if (is_numerical_op(op))
            return infer_numerical(op, lhs, rhs, types);

        if (is_logical_op(op))
            return infer_logical_op(op, lhs, rhs, types);

        return nullptr;
    }
}
