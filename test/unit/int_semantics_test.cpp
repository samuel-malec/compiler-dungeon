// Exhaustive check of ir/int_semantics.hpp. Every integer opcode is evaluated for every pair of
// 8-bit inputs, as both i8 and u8, and compared against a reference model written directly in
// C++ integer arithmetic. 8 bits is enough to exercise every code path (the width handling is
// generic), and small enough to check all 65536 input pairs. A few 32/64-bit edge cases cover
// the places where the 64-bit machinery itself could go wrong.
//
// basic_block.hpp has to come first: it and instruction.hpp include each other.
#include "../../src/ir/basic_block.hpp"
#include "../../src/ir/int_semantics.hpp"

#include <cstdio>

using namespace dungeon;
using namespace dungeon::ir;

namespace {
    int failures = 0;
    int checks = 0;

    const char *name(opcode op) {
        switch (op) {
            case opcode::add: return "add";
            case opcode::sub: return "sub";
            case opcode::mul: return "mul";
            case opcode::udiv: return "udiv";
            case opcode::umod: return "umod";
            case opcode::sdiv: return "sdiv";
            case opcode::smod: return "smod";
            case opcode::shl: return "shl";
            case opcode::shr: return "shr";
            case opcode::sar: return "sar";
            case opcode::eq: return "eq";
            case opcode::ult: return "ult";
            case opcode::ule: return "ule";
            case opcode::ugt: return "ugt";
            case opcode::uge: return "uge";
            case opcode::slt: return "slt";
            case opcode::sle: return "sle";
            case opcode::sgt: return "sgt";
            case opcode::sge: return "sge";
            default: return "?";
        }
    }

    void fail(const char *what, opcode op, const type &ty, uint64_t a, uint64_t b) {
        if (++failures <= 25)
            std::printf("FAIL %s %s%zu a=%lld b=%lld\n", what, name(op), ty.bits,
                        static_cast<long long>(a), static_cast<long long>(b));
    }

    // Reference model for 8-bit operands, given as raw bit patterns 0..255. Returns the
    // canonical 64-bit form of the result, or nullopt where the operation has no value.
    std::optional<uint64_t> ref_arith(opcode op, bool is_signed, int raw_a, int raw_b) {
        const int a = is_signed ? static_cast<int8_t>(raw_a) : raw_a;
        const int b = is_signed ? static_cast<int8_t>(raw_b) : raw_b;
        auto wrap = [&](int64_t x) -> uint64_t {
            const auto low = static_cast<uint8_t>(x & 0xFF);
            return is_signed ? static_cast<uint64_t>(static_cast<int64_t>(static_cast<int8_t>(low))) : low;
        };
        const bool shift_ok = b >= 0 && b < 8;

        switch (op) {
            case opcode::add: return wrap(a + b);
            case opcode::sub: return wrap(a - b);
            case opcode::mul: return wrap(a * b);
            case opcode::udiv:
            case opcode::sdiv:
                if (b == 0 || (is_signed && a == -128 && b == -1)) return std::nullopt;
                return wrap(a / b);
            case opcode::umod:
            case opcode::smod:
                if (b == 0 || (is_signed && a == -128 && b == -1)) return std::nullopt;
                return wrap(a % b);
            case opcode::shl:
                if (!shift_ok) return std::nullopt;
                return wrap(static_cast<int64_t>(static_cast<uint32_t>(a) << b));
            case opcode::shr:
                if (!shift_ok) return std::nullopt;
                return wrap((raw_a & 0xFF) >> b);
            case opcode::sar:
                if (!shift_ok) return std::nullopt;
                return wrap(a >> b);
            default:
                return std::nullopt;
        }
    }

    bool ref_cmp(opcode op, int a, int b) {
        switch (op) {
            case opcode::eq: return a == b;
            case opcode::ult: case opcode::slt: return a < b;
            case opcode::ule: case opcode::sle: return a <= b;
            case opcode::ugt: case opcode::sgt: return a > b;
            case opcode::uge: case opcode::sge: return a >= b;
            default: return false;
        }
    }

    void check_8bit(bool is_signed) {
        const type ty{is_signed ? type_kind::_int : type_kind::_uint, 8};

        // opcode and type must agree (see eval_arith), so each signedness gets its own ops
        const std::vector<opcode> arith = is_signed
            ? std::vector<opcode>{opcode::add, opcode::sub, opcode::mul, opcode::sdiv, opcode::smod,
                                  opcode::shl, opcode::sar}
            : std::vector<opcode>{opcode::add, opcode::sub, opcode::mul, opcode::udiv, opcode::umod,
                                  opcode::shl, opcode::shr};
        const std::vector<opcode> cmps = is_signed
            ? std::vector<opcode>{opcode::eq, opcode::slt, opcode::sle, opcode::sgt, opcode::sge}
            : std::vector<opcode>{opcode::eq, opcode::ult, opcode::ule, opcode::ugt, opcode::uge};

        for (int raw_a = 0; raw_a < 256; ++raw_a) {
            for (int raw_b = 0; raw_b < 256; ++raw_b) {
                const uint64_t a = normalize(raw_a, &ty);
                const uint64_t b = normalize(raw_b, &ty);

                for (opcode op: arith) {
                    ++checks;
                    const auto got = eval_arith(op, &ty, a, b);
                    const auto want = ref_arith(op, is_signed, raw_a, raw_b);
                    if (got != want)
                        fail("value", op, ty, a, b);
                    else if (got && normalize(*got, &ty) != *got)
                        fail("not canonical", op, ty, a, b);
                }

                const int va = is_signed ? static_cast<int8_t>(raw_a) : raw_a;
                const int vb = is_signed ? static_cast<int8_t>(raw_b) : raw_b;
                for (opcode op: cmps) {
                    ++checks;
                    if (eval_cmp(op, a, b) != ref_cmp(op, va, vb))
                        fail("cmp", op, ty, a, b);
                }
            }
        }
    }

    void expect(opcode op, const type &ty, uint64_t a, uint64_t b, std::optional<uint64_t> want) {
        ++checks;
        if (eval_arith(op, &ty, a, b) != want)
            fail("edge", op, ty, a, b);
    }
}

int main() {
    check_8bit(true);
    check_8bit(false);

    const type i32{type_kind::_int, 32}, i64{type_kind::_int, 64};
    const type u32{type_kind::_uint, 32}, u64{type_kind::_uint, 64};
    const uint64_t i64_min = uint64_t{1} << 63;
    const auto neg = [](int64_t v) { return static_cast<uint64_t>(v); };

    // 32 bit: the canonical form is sign-extended, so -8 is 0xFFFFFFFFFFFFFFF8
    expect(opcode::sar, i32, neg(-8), 1, neg(-4));
    expect(opcode::sar, i32, neg(-1), 31, neg(-1));
    expect(opcode::sar, i32, 5, 32, std::nullopt);
    expect(opcode::shl, i32, 1, 31, neg(INT32_MIN));
    expect(opcode::shl, i32, 1, 32, std::nullopt);
    expect(opcode::shr, u32, 0xFFFFFFFFu, 31, 1);
    expect(opcode::add, i32, 0x7FFFFFFF, 1, neg(INT32_MIN));
    expect(opcode::sub, u32, 0, 1, 0xFFFFFFFFu);
    expect(opcode::sdiv, i32, neg(INT32_MIN), neg(-1), std::nullopt);

    // 64 bit: nothing to truncate, the 64-bit operations themselves must not be undefined
    expect(opcode::shl, i64, 1, 63, i64_min);
    expect(opcode::shl, i64, 1, 64, std::nullopt);
    expect(opcode::sar, i64, i64_min, 63, neg(-1));
    expect(opcode::sar, i64, i64_min, 64, std::nullopt);
    expect(opcode::shr, u64, UINT64_MAX, 63, 1);
    expect(opcode::sdiv, i64, i64_min, neg(-1), std::nullopt);
    expect(opcode::smod, i64, i64_min, neg(-1), std::nullopt);
    expect(opcode::add, i64, neg(INT64_MAX), 1, i64_min);
    expect(opcode::add, u64, UINT64_MAX, 1, 0);
    expect(opcode::udiv, u64, UINT64_MAX, 2, UINT64_MAX / 2);

    std::printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
