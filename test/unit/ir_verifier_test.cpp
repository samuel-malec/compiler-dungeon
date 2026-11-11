// Tests for src/analysis/ir_verifier.hpp -- by MUTATION.
//
// How to test a checker: you can't only feed it good input (a verifier that accepts everything
// passes that). So this file
//   1. builds small, hand-made, VALID functions ("scenarios") and checks the verifier accepts them;
//   2. for each "mutation", builds a fresh valid scenario, breaks it in exactly one way, and checks
//      the verifier reports the rule that was broken.
//
// Your job: replace each `TODO` below. A mutation is a lambda that edits the scenario and returns
// true (it returns false until you write it, which counts as a failure). Think of it as the other
// half of the assignment -- every rule in verifier_rule deserves a way to break it.
//
// Building blocks you have:
//   scenario::B("name") / V("name") / I("name")   named blocks, values and instructions
//   the builders make_diamond(), make_loop(), make_memory() below show every handle's name
//   replace_operand(inst, index, new_value)       swaps an operand and keeps the use-lists correct,
//                                                 so only the rule you are testing breaks
//   erase_from_block(inst)                        removes an instruction from its block (but not
//                                                 from fn.instructions, which owns it)
//
// Run:  ctest -L verifier --output-on-failure      (or ./build/ir_verifier_test)
//
// basic_block.hpp has to come first: it and instruction.hpp include each other.
#include "../../src/ir/basic_block.hpp"
#include "../../src/analysis/ir_verifier.hpp"

#include <cstdio>
#include <memory>
#include <string>

#include <sys/wait.h>
#include <unistd.h>

using namespace dungeon;

// ======================================================================================
// A tiny IR builder (given -- you don't need to change it)
// ======================================================================================

struct scenario {
    type_manager types; // the values point into this, so it lives as long as the function
    ir::function fn;
    uint32_t next_value = 1;

    std::map<std::string, basic_block *> blocks;
    std::map<std::string, ir::value *> values;
    std::map<std::string, ir::instruction *> instrs;

    basic_block *B(const std::string &n) const { return blocks.at(n); }
    ir::value *V(const std::string &n) const { return values.at(n); }
    ir::instruction *I(const std::string &n) const { return instrs.at(n); }

    basic_block *block(const std::string &n) {
        auto b = std::make_unique<basic_block>();
        b->id = block_id{static_cast<uint32_t>(fn.blocks.size())};
        fn.blocks.push_back(std::move(b));
        if (!fn.entry)
            fn.entry = fn.blocks.back().get();
        return blocks[n] = fn.blocks.back().get();
    }

    ir::value *value(const std::string &n, const type *ty) {
        fn.values.push_back(std::make_unique<ir::value>(next_value++, ty, std::vector<ir::instruction *>{}));
        return values[n] = fn.values.back().get();
    }

    // Appends an instruction to `b`, wiring up parent, defining_instruction and the use-lists.
    ir::instruction *emit(const std::string &n, basic_block *b, ir::opcode op, ir::value *result,
                          std::vector<ir::value *> operands, ir::instruction::data_t data = {}) {
        auto ins = std::make_unique<ir::instruction>();
        ins->op = op;
        ins->result = result;
        ins->operands = std::move(operands);
        ins->data = std::move(data);
        ins->parent = b;
        if (result)
            result->defining_instruction = ins.get();
        for (ir::value *o: ins->operands)
            o->users.push_back(ins.get());
        b->instructions.push_back(ins.get());
        fn.instructions.push_back(std::move(ins));
        return instrs[n] = fn.instructions.back().get();
    }

    void edge(basic_block *from, basic_block *to) {
        from->succ.push_back(to);
        to->pred.push_back(from);
    }

    void br(const std::string &n, basic_block *from, basic_block *to) {
        emit(n, from, ir::opcode::br, nullptr, {}, ir::br_data{to->id.id});
        edge(from, to);
    }

    void cond_br(const std::string &n, basic_block *from, ir::value *cond, basic_block *t, basic_block *f) {
        emit(n, from, ir::opcode::cond_br, nullptr, {cond}, ir::cond_br_data{t->id.id, f->id.id});
        edge(from, t);
        edge(from, f);
    }

    ir::instruction *phi(const std::string &n, basic_block *b, ir::value *result,
                         std::vector<std::pair<basic_block *, ir::value *>> incoming) {
        ir::phi_data data;
        std::vector<ir::value *> operands;
        for (auto &[pred, v]: incoming) {
            data.incoming[pred] = v;
            operands.push_back(v);
        }
        return emit(n, b, ir::opcode::phi, result, std::move(operands), std::move(data));
    }
};

// Swaps operand `index` of `inst` for `v`, keeping both use-lists exact.
void replace_operand(ir::instruction *inst, size_t index, ir::value *v) {
    inst->erase_use(inst->operands[index]); // removes ALL entries for this user from the old value
    ir::value *old = inst->operands[index];
    inst->operands[index] = v;
    v->users.push_back(inst);
    // erase_use removed every entry of `inst`, so re-add the ones for other operand slots
    for (size_t k = 0; k < inst->operands.size(); ++k)
        if (k != index && inst->operands[k] == old)
            old->users.push_back(inst);
}

void erase_from_block(ir::instruction *inst) {
    auto &v = inst->parent->instructions;
    v.erase(std::find(v.begin(), v.end(), inst));
}

// ======================================================================================
// Scenarios: valid functions, one per interesting shape.   Handle names are in quotes.
// ======================================================================================

// fn(a: i32) -> i32:   (a < 0) ? 10 : 20
//
//   entry: %a = param; %zero = iconst 0; %cond = slt %a, %zero; cond_br %cond, then, else
//   then:  %ten = iconst 10; br merge
//   else:  %twenty = iconst 20; br merge
//   merge: %merged = phi [then: %ten], [else: %twenty]; ret %merged
std::unique_ptr<scenario> make_diamond() {
    auto s = std::make_unique<scenario>();
    const type *i32 = s->types.get_int(32);
    s->fn.return_type = i32;
    s->fn.param_types = {i32};

    auto *entry = s->block("entry"), *then_ = s->block("then"), *else_ = s->block("else"), *merge = s->block("merge");
    auto *a = s->value("a", i32), *zero = s->value("zero", i32), *cond = s->value("cond", s->types.get_bool());
    auto *ten = s->value("ten", i32), *twenty = s->value("twenty", i32), *merged = s->value("merged", i32);

    s->emit("param", entry, ir::opcode::param, a, {}, ir::param_data{0});
    s->emit("zero", entry, ir::opcode::iconst, zero, {}, ir::iconst_data{0});
    s->emit("slt", entry, ir::opcode::slt, cond, {a, zero});
    s->cond_br("cond_br", entry, cond, then_, else_);

    s->emit("ten", then_, ir::opcode::iconst, ten, {}, ir::iconst_data{10});
    s->br("then_br", then_, merge);

    s->emit("twenty", else_, ir::opcode::iconst, twenty, {}, ir::iconst_data{20});
    s->br("else_br", else_, merge);

    s->phi("phi", merge, merged, {{then_, ten}, {else_, twenty}});
    s->emit("ret", merge, ir::opcode::ret, nullptr, {merged});
    return s;
}

// fn(n: i32) -> i32:   i = 0; sum = 0; while i < n { sum += i; i += 1 }; sum
//
//   entry: %n = param; %zero = iconst 0; br header
//   header: %sum = phi [entry: %zero], [body: %sum2]
//           %i   = phi [entry: %zero], [body: %i2]
//           %cond = slt %i, %n; cond_br %cond, body, exit
//   body:  %one = iconst 1; %sum2 = add %sum, %i; %i2 = add %i, %one; br header
//   exit:  ret %sum
std::unique_ptr<scenario> make_loop() {
    auto s = std::make_unique<scenario>();
    const type *i32 = s->types.get_int(32);
    s->fn.return_type = i32;
    s->fn.param_types = {i32};

    auto *entry = s->block("entry"), *header = s->block("header"), *body = s->block("body"), *exit = s->block("exit");
    auto *n = s->value("n", i32), *zero = s->value("zero", i32);
    auto *sum = s->value("sum", i32), *i = s->value("i", i32), *cond = s->value("cond", s->types.get_bool());
    auto *one = s->value("one", i32), *sum2 = s->value("sum2", i32), *i2 = s->value("i2", i32);

    s->emit("param", entry, ir::opcode::param, n, {}, ir::param_data{0});
    s->emit("zero", entry, ir::opcode::iconst, zero, {}, ir::iconst_data{0});
    s->br("entry_br", entry, header);

    s->phi("sum_phi", header, sum, {{entry, zero}, {body, sum2}});
    s->phi("i_phi", header, i, {{entry, zero}, {body, i2}});
    s->emit("slt", header, ir::opcode::slt, cond, {i, n});
    s->cond_br("cond_br", header, cond, body, exit);

    s->emit("one", body, ir::opcode::iconst, one, {}, ir::iconst_data{1});
    s->emit("add_sum", body, ir::opcode::add, sum2, {sum, i});
    s->emit("add_i", body, ir::opcode::add, i2, {i, one});
    s->br("body_br", body, header);

    s->emit("ret", exit, ir::opcode::ret, nullptr, {sum});
    return s;
}

// The memory form, before mem2reg:
//   fn() -> i32:   %slot = alloca; %seven = iconst 7; store %slot, %seven; %v = load %slot; ret %v
std::unique_ptr<scenario> make_memory() {
    auto s = std::make_unique<scenario>();
    const type *i32 = s->types.get_int(32);
    s->fn.return_type = i32;

    auto *entry = s->block("entry");
    auto *slot = s->value("slot", i32), *seven = s->value("seven", i32), *v = s->value("v", i32);

    s->emit("alloca", entry, ir::opcode::alloca, slot, {});
    s->emit("seven", entry, ir::opcode::iconst, seven, {}, ir::iconst_data{7});
    s->emit("store", entry, ir::opcode::store, nullptr, {slot, seven});
    s->emit("load", entry, ir::opcode::load, v, {slot});
    s->emit("ret", entry, ir::opcode::ret, nullptr, {v});
    return s;
}

// A block that branches to itself is perfectly valid: it is its own predecessor AND successor.
//   fn(a: i32) -> i32:   spin until a == a (i.e. one iteration), then return a
//
//   entry: %a = param; br spin
//   spin:  %c = eq %a, %a; cond_br %c, spin, exit
//   exit:  ret %a
std::unique_ptr<scenario> make_self_loop() {
    auto s = std::make_unique<scenario>();
    const type *i32 = s->types.get_int(32);
    s->fn.return_type = i32;
    s->fn.param_types = {i32};

    auto *entry = s->block("entry"), *spin = s->block("spin"), *exit = s->block("exit");
    auto *a = s->value("a", i32), *c = s->value("c", s->types.get_bool());

    s->emit("param", entry, ir::opcode::param, a, {}, ir::param_data{0});
    s->br("entry_br", entry, spin);

    s->emit("eq", spin, ir::opcode::eq, c, {a, a});
    s->cond_br("cond_br", spin, c, spin, exit);

    s->emit("ret", exit, ir::opcode::ret, nullptr, {a});
    return s;
}

// ======================================================================================
// Mutations
// ======================================================================================

struct mutation {
    const char *name;
    const char *rule;                          // the rule that must fire (nullptr: any error will do)
    std::unique_ptr<scenario> (*make)();       // which valid function to start from
    bool (*apply)(scenario &);                 // breaks it; false = not written yet
};

const mutation mutations[] = {
    // ---------------------------------------------------------------- worked examples (given)
    {"missing_terminator", verifier_rule::terminator, make_diamond,
     [](scenario &s) {
         s.B("then")->instructions.pop_back(); // the `br merge` disappears
         return true;
     }},

    {"asymmetric_edge", verifier_rule::edge_symmetry, make_diamond,
     [](scenario &s) {
         // `then` still lists `merge` as a successor, but `merge` forgot about `then`
         auto &pred = s.B("merge")->pred;
         pred.erase(std::find(pred.begin(), pred.end(), s.B("then")));
         return true;
     }},

    {"stale_user", verifier_rule::use_list, make_diamond,
     [](scenario &s) {
         // `ret` is recorded as a user of %zero, but it doesn't use it
         s.V("zero")->users.push_back(s.I("ret"));
         return true;
     }},

    // ---------------------------------------------------------------- Part 1: CFG shape
    {"duplicate_block_id", verifier_rule::unique_block_id, make_diamond,
     [](scenario &s) {
         s.B("else")->id = s.B("then")->id;
         return true;
     }},

    {"duplicate_pred", verifier_rule::edge_dup, make_diamond,
     [](scenario &s) {
         s.B("merge")->pred.push_back(s.B("then"));
         return true;
     }},

    {"successor_outside_function", verifier_rule::edge_target, make_diamond,
     [](scenario &s) {
         static basic_block stranger; // lives forever, belongs to no function
         stranger.id = block_id{99};
         s.B("then")->succ.push_back(&stranger);
         return true;
     }},

    {"branch_back_to_entry", verifier_rule::entry_no_preds, make_loop,
     [](scenario &s) {
         // make `body` branch to the entry instead of the header, keeping the edge lists consistent
         auto *body = s.B("body");
         auto *header = s.B("header");
         body->succ.clear();
         header->pred.erase(std::find(header->pred.begin(), header->pred.end(), body));
         s.edge(body, s.B("entry"));
         return true;
     }},

    // ---------------------------------------------------------------- Part 2: blocks and instructions
    {"wrong_parent", verifier_rule::parent, make_diamond,
     [](scenario &s) {
         s.I("ten")->parent = s.B("else");
         return true;
     }},

    {"not_owned", verifier_rule::owned, make_diamond,
     [](scenario &s) {
         static ir::instruction orphan;
         orphan.op = ir::opcode::br;
         orphan.parent = s.B("then");
         s.B("then")->instructions.back() = &orphan; // replaces the br with one nobody owns
         return true;
     }},

    {"terminator_in_the_middle", verifier_rule::terminator, make_diamond,
     [](scenario &s) {
         // put a copy of the br before the iconst
         s.B("then")->instructions.insert(s.B("then")->instructions.begin(), s.I("then_br"));
         return true;
     }},

    {"br_with_two_successors", verifier_rule::succ_count, make_diamond,
     [](scenario &s) {
         s.edge(s.B("then"), s.B("else"));
         return true;
     }},

    {"ret_with_a_successor", verifier_rule::succ_count, make_diamond,
     [](scenario &s) {
         s.edge(s.B("merge"), s.B("then"));
         return true;
     }},

    // ---------------------------------------------------------------- Part 3: values and use-lists
    {"result_points_elsewhere", verifier_rule::def_link, make_diamond,
     [](scenario &s) {
         s.V("ten")->defining_instruction = s.I("twenty");
         return true;
     }},

    {"defined_twice", verifier_rule::single_def, make_diamond,
     [](scenario &s) {
         s.I("twenty")->result = s.V("ten"); // two iconsts now both define %ten
         return true;
     }},

    {"null_operand", verifier_rule::operand_null, make_diamond,
     [](scenario &s) {
         s.I("slt")->operands[1] = nullptr;
         return true;
     }},

    {"operand_defined_by_erased_instruction", verifier_rule::operand_def, make_diamond,
     [](scenario &s) {
         erase_from_block(s.I("zero")); // %zero's definition is gone, but `slt` still uses it
         return true;
     }},

    {"use_missing_from_use_list", verifier_rule::use_list, make_diamond,
     [](scenario &s) {
         auto &users = s.V("zero")->users;
         users.erase(std::find(users.begin(), users.end(), s.I("slt")));
         return true;
     }},

    {"use_listed_once_but_used_twice", verifier_rule::use_list, make_loop,
     [](scenario &s) {
         // `add_sum = add %sum, %i` and `slt = slt %i, %n` both use %i; make add_sum use %i twice
         // but leave the use-list alone
         s.I("add_sum")->operands[0] = s.V("i");
         return true;
     }},

    // ---------------------------------------------------------------- Part 4: types
    {"add_result_is_bool", verifier_rule::type, make_loop,
     [](scenario &s) {
         s.V("sum2")->ty = s.types.get_bool();
         return true;
     }},

    {"cond_br_on_an_integer", verifier_rule::type, make_diamond,
     [](scenario &s) {
         replace_operand(s.I("cond_br"), 0, s.V("a"));
         return true;
     }},

    {"ret_wrong_type", verifier_rule::type, make_diamond,
     [](scenario &s) {
         s.fn.return_type = s.types.get_bool();
         return true;
     }},

    {"load_from_a_non_slot", verifier_rule::type, make_memory,
     [](scenario &s) {
         replace_operand(s.I("load"), 0, s.V("seven")); // loading from an iconst
         return true;
     }},

    {"unsigned_compare_on_signed_type", verifier_rule::signedness, make_diamond,
     [](scenario &s) {
         s.I("slt")->op = ir::opcode::ult; // %a and %zero are i32
         return true;
     }},

    {"param_outside_entry", verifier_rule::param_entry, make_diamond,
     [](scenario &s) {
         erase_from_block(s.I("param"));
         s.I("param")->parent = s.B("then");
         s.B("then")->instructions.insert(s.B("then")->instructions.begin(), s.I("param"));
         return true;
     }},

    // ---------------------------------------------------------------- Part 5: phis and dominance
    {"phi_after_non_phi", verifier_rule::phi_first, make_loop,
     [](scenario &s) {
         // move the second phi behind the slt
         auto &v = s.B("header")->instructions;
         v.erase(std::find(v.begin(), v.end(), s.I("i_phi")));
         v.insert(v.begin() + 2, s.I("i_phi"));
         return true;
     }},

    {"phi_missing_predecessor", verifier_rule::phi_preds, make_diamond,
     [](scenario &s) {
         auto *phi = s.I("phi");
         std::get<ir::phi_data>(phi->data).incoming.erase(s.B("else"));
         phi->erase_use(s.V("twenty"));
         phi->operands.erase(std::find(phi->operands.begin(), phi->operands.end(), s.V("twenty")));
         return true;
     }},

    {"phi_incoming_from_a_non_predecessor", verifier_rule::phi_preds, make_diamond,
     [](scenario &s) {
         auto *phi = s.I("phi");
         std::get<ir::phi_data>(phi->data).incoming[s.B("entry")] = s.V("zero");
         phi->operands.push_back(s.V("zero"));
         s.V("zero")->users.push_back(phi);
         return true;
     }},

    {"phi_operands_out_of_sync", verifier_rule::phi_operands, make_diamond,
     [](scenario &s) {
         // change what the map says, but not `operands`
         std::get<ir::phi_data>(s.I("phi")->data).incoming[s.B("then")] = s.V("twenty");
         return true;
     }},

    {"use_before_definition", verifier_rule::dominance, make_diamond,
     [](scenario &s) {
         // `slt` uses %zero, which is defined by the instruction before it: swap them
         auto &v = s.B("entry")->instructions;
         std::swap(v[1], v[2]);
         return true;
     }},

    {"use_not_dominated", verifier_rule::dominance, make_diamond,
     [](scenario &s) {
         // `merge` is not dominated by `then`, so it can't just use %ten
         replace_operand(s.I("ret"), 0, s.V("ten"));
         return true;
     }},

    {"phi_operand_does_not_dominate_predecessor", verifier_rule::dominance, make_diamond,
     [](scenario &s) {
         // the value arriving from `then` is defined in `else`
         auto *phi = s.I("phi");
         std::get<ir::phi_data>(phi->data).incoming[s.B("then")] = s.V("twenty");
         phi->erase_use(s.V("ten"));
         phi->operands = {s.V("twenty"), s.V("twenty")};
         s.V("twenty")->users = {phi, phi};
         return true;
     }},

    // ---------------------------------------------------------------- robustness: must not crash
    // A verifier is given broken IR by definition. These two put a null pointer where the
    // verifier would naturally dereference without looking.
    {"null_successor", verifier_rule::edge_target, make_diamond,
     [](scenario &s) {
         s.B("then")->succ.push_back(nullptr);
         return true;
     }},

    {"null_instruction_in_block", nullptr, make_diamond,
     [](scenario &s) {
         auto &v = s.B("then")->instructions;
         v.insert(v.begin(), nullptr);
         return true;
     }},
};

// ======================================================================================
// Runner (given)
// ======================================================================================

namespace {
    bool has_rule(const std::vector<verify_error> &errors, const char *rule) {
        return std::any_of(errors.begin(), errors.end(), [&](const verify_error &e) { return e.rule == rule; });
    }

    // Prints the first few errors only: a broken check tends to repeat itself on every edge.
    void print_errors(const std::vector<verify_error> &errors) {
        constexpr size_t limit = 5;
        for (size_t k = 0; k < errors.size() && k < limit; ++k)
            std::printf("        [%s] %s\n", errors[k].rule.c_str(), errors[k].message.c_str());
        if (errors.size() > limit)
            std::printf("        ... and %zu more\n", errors.size() - limit);
    }

    // A verifier is given broken IR by definition, so it must never crash on it. While you are
    // still writing it, it will -- so run it in a child process first: a crash fails THAT test
    // (CRASH) instead of ending the whole run. (POSIX only, like the rest of this project's tests.)
    bool crashes(const ir::function &fn) {
        std::fflush(stdout);
        const pid_t pid = fork();
        if (pid == 0) {
            ir_verifier::verify(fn);
            _exit(0);
        }
        int status = 0;
        waitpid(pid, &status, 0);
        return WIFSIGNALED(status);
    }
}

int main() {
    int failed = 0, todo = 0, passed = 0;

    // 1. valid functions must be accepted
    const struct { const char *name; std::unique_ptr<scenario> (*make)(); } baselines[] = {
        {"diamond", make_diamond}, {"loop", make_loop}, {"memory", make_memory}, {"self_loop", make_self_loop},
    };
    for (const auto &b: baselines) {
        auto s = b.make();
        if (crashes(s->fn)) {
            ++failed;
            std::printf("CRASH valid '%s' function crashed the verifier\n", b.name);
            continue;
        }
        auto errors = ir_verifier::verify(s->fn);
        if (errors.empty()) {
            ++passed;
        } else {
            ++failed;
            std::printf("FAIL  valid '%s' function was rejected:\n", b.name);
            print_errors(errors);
        }
    }

    // 2. every mutation must be caught, with the right rule
    for (const auto &m: mutations) {
        auto s = m.make();
        if (!m.apply(*s)) {
            ++todo;
            std::printf("TODO  %s\n", m.name);
            continue;
        }
        if (crashes(s->fn)) {
            ++failed;
            std::printf("CRASH %s: the verifier crashed on it\n", m.name);
            continue;
        }
        auto errors = ir_verifier::verify(s->fn);
        if (m.rule ? has_rule(errors, m.rule) : !errors.empty()) {
            ++passed;
        } else {
            ++failed;
            std::printf("FAIL  %s: expected rule [%s]; got %zu error(s)\n", m.name, m.rule ? m.rule : "any",
                        errors.size());
            print_errors(errors);
        }
    }

    std::printf("\n%d passed, %d failed, %d mutations still to write\n", passed, failed, todo);
    return failed == 0 && todo == 0 ? 0 : 1;
}
