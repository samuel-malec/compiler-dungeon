#pragma once

#include "../frontend/ast.hpp"

namespace dungeon {
    enum class type_kind {
        _int,
        _uint,
        _bool,
        _unit,
    };

    enum class op_category {
        numeric,
        ordering,
        equality,
        logical,
        unary_numeric,
        unary_logical
    };

    struct type {
        type_kind kind;
        size_t bits;
    };

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

    inline bool same_type(const type *a, const type *b) {
        return a == b;
    }

    inline bool compatible_types(const type *a, const type *b) {
        return a == b;
    }

    inline bool is_boolean_ty(const type *ty) {
        return ty->kind == type_kind::_bool;
    }

    inline bool is_signed_integer_ty(const type *ty) {
        return ty->kind == type_kind::_int;
    }

    inline bool is_unsigned_integer_ty(const type *ty) {
        return ty->kind == type_kind::_uint;
    }

    inline bool is_integer_ty(const type *ty) {
        return is_signed_integer_ty(ty) ||
               is_unsigned_integer_ty(ty);
    }

    inline bool is_unit_ty(const type *ty) {
        return ty->kind == type_kind::_unit;
    }

    inline op_category category_of(op_kind op) {
        switch (op) {
            case ADD:
            case SUB:
            case MUL:
            case DIV:
            case MOD:
            case SHL:
            case SHR:
                return op_category::numeric;
            case EQ:
            case NEQ:
                return op_category::equality;
            case LT:
            case LEQ:
            case GT:
            case GEQ:
                return op_category::ordering;
            case NOT:
                return op_category::unary_logical;
            case MINUS:
            case PLUS:
                return op_category::unary_numeric;
            case AND:
            case OR:
                return op_category::logical;
            default: assert(false && "should not reach here");
        }
    }

    inline bool operand_ok(op_category cat, const type *t) {
        switch (cat) {
            case op_category::numeric:
                return is_integer_ty(t);
            case op_category::ordering:
                return is_integer_ty(t);
            case op_category::equality:
                return true;
            case op_category::logical:
                return is_boolean_ty(t);
            case op_category::unary_numeric:
                return is_integer_ty(t);
            case op_category::unary_logical:
                return is_boolean_ty(t);
        }
        return false;
    }

    inline const type *result_type(op_category cat, const type *operand, type_manager &types) {
        if (cat == op_category::numeric || cat == op_category::unary_numeric)
            return operand;
        if (cat == op_category::unary_logical || cat == op_category::logical)
            return operand;
        if ( cat == op_category::equality || cat == op_category::ordering)
            return types.get_bool();
        assert(false && "should not reach here");
    }

    inline const type *infer_op(op_kind op, const type *lhs, const type *rhs, type_manager &types) {
        auto cat = category_of(op);
        if (!operand_ok(cat, lhs) || (rhs && !operand_ok(cat, rhs)))
            diag::error("Invalid operand type for operator", op);
        if (rhs && !same_type(lhs, rhs))
            diag::error("Mismatched operand types", op);
        return result_type(cat, lhs, types);
    }
}
