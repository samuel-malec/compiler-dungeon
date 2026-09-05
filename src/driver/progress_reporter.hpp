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
                --r.current_depth;
                r.entries.push_back({name, std::chrono::steady_clock::now() - start, r.current_depth});
            }
        };

        // We have use std::cerr here, because some tests require comparing actual/expected std::cout output from compiler pipeline
        void pad(int depth) {
            for (int i = 0; i < 2 * depth; ++i)
                std::cerr << " ";
        }

        ~progress_reporter() {
            // TODO: add a percentage of time here
            for (auto &e: entries) {
                pad(e.depth);
                std::cerr << '[' << e.name << "] ";
                std::cerr << "took " << e.duration.count() << " ms" << '\n';
            }
        }

        scope time(std::string name) { return scope(*this, std::move(name)); }

        void print(std::ostream &out) const;
    };
}
