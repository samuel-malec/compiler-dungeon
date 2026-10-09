#pragma once

#include <cassert>
#include <cstdint>
#include <optional>

#include "instruction.hpp"
#include "../sema/types.hpp"

namespace dungeon::ir {
    inline bool is_comparison(opcode op) {
        switch (op) {
            case opcode::eq:
            case opcode::slt: case opcode::sle: case opcode::sgt: case opcode::sge:
            case opcode::ult: case opcode::ule: case opcode::ugt: case opcode::uge:
                return true;
            default:
                return false;
        }
    }

    // Truncates `v` to the width of `ty`, then extends it back to 64 bits per its signedness.
    inline uint64_t normalize(uint64_t v, const type *ty) {
        assert(is_integer_ty(ty));
        const size_t bits = ty->bits;
        if (bits >= 64)
            return v;
        const uint64_t mask = (uint64_t{1} << bits) - 1;
        v &= mask;
        if (is_signed_integer_ty(ty) && (v >> (bits - 1)) & 1)
            v |= ~mask;
        return v;
    }

    inline bool eval_cmp(opcode op, uint64_t a, uint64_t b) {
        const auto sa = static_cast<int64_t>(a);
        const auto sb = static_cast<int64_t>(b);
        switch (op) {
            case opcode::eq: return a == b;
            case opcode::ult: return a < b;
            case opcode::ule: return a <= b;
            case opcode::ugt: return a > b;
            case opcode::uge: return a >= b;
            case opcode::slt: return sa < sb;
            case opcode::sle: return sa <= sb;
            case opcode::sgt: return sa > sb;
            case opcode::sge: return sa >= sb;
            default: assert(false && "not a comparison opcode"); return false;
        }
    }

    inline std::optional<uint64_t> eval_arith(opcode op, const type *ty, uint64_t a, uint64_t b) {
        const auto sa = static_cast<int64_t>(a);
        const auto sb = static_cast<int64_t>(b);
        const size_t bits = ty->bits;

        switch (op) {
            case opcode::add: return normalize(a + b, ty);
            case opcode::sub: return normalize(a - b, ty);
            case opcode::mul: return normalize(a * b, ty);

            case opcode::udiv:
                if (b == 0) return std::nullopt;
                return normalize(a / b, ty);
            case opcode::umod:
                if (b == 0) return std::nullopt;
                return normalize(a % b, ty);

            case opcode::sdiv:
            case opcode::smod: {
                if (sb == 0) return std::nullopt;
                const auto min_value = static_cast<int64_t>(normalize(uint64_t{1} << (bits - 1), ty));
                if (sa == min_value && sb == -1) return std::nullopt;
                return normalize(static_cast<uint64_t>(op == opcode::sdiv ? sa / sb : sa % sb), ty);
            }

            case opcode::shl:
                if (b >= bits) return std::nullopt;
                return normalize(a << b, ty);
            case opcode::shr:
                if (b >= bits) return std::nullopt;
                return is_signed_integer_ty(ty) ? normalize(static_cast<uint64_t>(sa >> b), ty) : a >> b;

            default:
                assert(false && "not an arithmetic opcode");
                return std::nullopt;
        }
    }
}
