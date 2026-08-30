#pragma once
#include <cassert>
#include <iosfwd>

#include "../diag/diag.hpp"

namespace dungeon {
    enum op_kind {
        ADD, SUB, MUL, DIV, MOD, SHL, SHR,

        ADD_EQ, SUB_EQ, MUL_EQ, DIV_EQ, MOD_EQ, SHL_EQ, SHR_EQ,

        EQ, NEQ, LT, LEQ, GT, GEQ,

        NOT, MINUS, PLUS,

        AND, OR,
    };

    // In these functions, we omit the operations representing compound assignments, since these get desugared after parsing
    inline bool is_rel_op(op_kind op) {
        return op == EQ || op == NEQ || op == LT || op == LEQ || op == GT || op == GEQ;
    }

    inline bool is_equality_op(op_kind op) {
        return op == EQ || op == NEQ;
    }

    inline bool is_ordering_op(op_kind op) {
        return op == LT || op == LEQ || op == GT || op == GEQ;
    }

    inline bool is_logical_op(op_kind op) {
        return op == AND || op == OR || op == NOT;
    }

    inline bool is_unary_op(op_kind op) {
        return op == NOT || op == MINUS || op == PLUS;
    }

    inline bool is_binary_op(op_kind op) {
        return op == ADD || op == SUB || op == MUL || op == DIV || op == MOD ||
               op == SHL || op == SHR || op == EQ || op == NEQ || op == LT || op == LEQ;
    }

    inline bool is_numerical_op(op_kind op) {
        return op == ADD || op == SUB || op == MUL ||
               op == DIV || op == MOD || op == SHL || op == SHR;
    }

    inline op_kind op_from_compound_asn(op_kind op) {
        if (op == ADD_EQ) return ADD;
        if (op == SUB_EQ) return SUB;
        if (op == MUL_EQ) return MUL;
        if (op == DIV_EQ) return DIV;
        if (op == MOD_EQ) return MOD;
        if (op == SHL_EQ) return SHL;
        if (op == SHR_EQ) return SHR;
        assert(false && "should not reach here, expected a compound assignment!");
    }

    inline op_kind op_kind_from_str(std::string_view data, bool is_unary = false ) {
        if ( data == "+" ) return is_unary ? PLUS : ADD;
        if ( data == "-" ) return is_unary ? MINUS : SUB;
        if (data == "*") return MUL;
        if (data == "/") return DIV;
        if (data == "%") return MOD;
        if (data == "<<") return SHL;
        if (data == ">>") return SHR;
        if (data == "==") return EQ;
        if (data == "!=") return NEQ;
        if (data == "<") return LT;
        if (data == "<=") return LEQ;
        if (data == ">") return GT;
        if (data == ">=") return GEQ;
        if (data == "!") return NOT;
        if (data == "&&") return AND;
        if (data == "||") return OR;
        if (data == "=") return EQ;
        if (data == "+=") return ADD_EQ;
        if (data == "-=") return SUB_EQ;
        if (data == "*=") return MUL_EQ;
        if (data == "%=") return MOD_EQ;
        if (data == "/=") return DIV_EQ;
        if (data == "<<=") return SHL_EQ;
        if (data == ">>=") return SHR_EQ;
        diag::error("Unknown operator:", data);
        return ADD;
    }

    inline std::ostream &operator<<(std::ostream &os, const op_kind op) {
        switch (op) {
            case ADD: return os << "+";
            case SUB: return os << "-";
            case PLUS: return os << "+";
            case MINUS: return os << "-";
            case MUL: return os << "*";
            case DIV: return os << "/";
            case MOD: return os << "%";
            case SHL: return os << "<<";
            case SHR: return os << ">>";
            case EQ: return os << "==";
            case NEQ: return os << "!=";
            case LT: return os << "<";
            case LEQ: return os << "<=";
            case GT: return os << ">";
            case GEQ: return os << ">=";
            case NOT: return os << "!";
            case AND: return os << "&&";
            case OR: return os << "||";
            case ADD_EQ: return os << "+=";
            case SUB_EQ: return os << "-=";
            case MUL_EQ: return os << "*=";
            case DIV_EQ: return os << "/=";
            case MOD_EQ: return os << "%=";
            case SHL_EQ: return os << "<<=";
            case SHR_EQ: return os << ">>=";
        }

        return os << "idk";
    }
}
