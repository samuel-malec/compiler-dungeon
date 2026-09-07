#pragma once
#include <chrono>
#include <string>

namespace dungeon {
    struct timed_entry {
        std::string name;
        std::chrono::nanoseconds duration;
        int depth;
    };

    struct progress_reporter {
        std::vector<timed_entry> entries;
        int current_depth = 0;

        struct scope {
            progress_reporter &r;
            std::string name;
            std::chrono::steady_clock::time_point start;

            scope(progress_reporter &r, std::string name) : r{r}, name{std::move(name)},
                                                            start{std::chrono::steady_clock::now()} {
                ++r.current_depth;
            }

            ~scope() {
                r.entries.push_back({name, std::chrono::steady_clock::now() - start, r.current_depth});
                --r.current_depth;
            }
        };

        // FIXME: Currently, we have use std::cerr here, because some tests require comparing actual/expected std::cout output from compiler pipeline
        std::string indent(std::ostream &out, int depth) {
            return std::string(depth * 2, ' ');
        }

        void pad(std::ostream &out, int depth) {
            out << indent(out, depth);
        }

        ~progress_reporter() {
            std::cout << "\033[1;32mCompilation report:\033[m\n";
            for (auto &e: entries) {
                pad(std::cout, e.depth);
                std::cout << '[' << e.name << "] ";
                std::cout << "took " << e.duration.count() << " ms" << '\n';
            }
        }

        scope time(std::string name) { return scope(*this, std::move(name)); }

        void print(std::ostream &out) const;
    };
}
