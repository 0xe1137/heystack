//
// Created by elliot on 9/20/26.
//

#ifndef HEYSTACK_DEBUGTIMER_H
#define HEYSTACK_DEBUGTIMER_H

#include <chrono>
#include <iostream>
#include <string_view>

class DebugTimer {
public:
    using Clock = std::chrono::steady_clock;

    explicit DebugTimer(std::string label)
        : label_(std::move(label)) {
        start();
    }

    DebugTimer(std::string label, bool auto_start)
        : label_(std::move(label)) {
        if (auto_start) {
            start();
        }
    }

    ~DebugTimer() {
        if (running_) {
            stop();
        }
    }

    void start() {
        start_ = Clock::now();
        running_ = true;
    }

    double stop() {
        if (!running_) {
            return elapsed_ms_;
        }

        const auto end = Clock::now();

        elapsed_ms_ =
            std::chrono::duration<double, std::milli>(
                end - start_
            ).count();

        running_ = false;

        std::cout
            << "[TIMER] "
            << label_
            << ": "
            << elapsed_ms_
            << " ms\n";

        return elapsed_ms_;
    }

private:
    std::string label_;
    Clock::time_point start_{};
    bool running_{false};
    double elapsed_ms_{0.0};
};

#endif //HEYSTACK_DEBUGTIMER_H

