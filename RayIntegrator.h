//
// Created by AlexandrGeorgiev on 05.08.2026.
//
#pragma once
struct State {
    double u, w;
    State operator+(const State &other) const;
    State operator*(double d) const;
    State(double u, double w): u(u), w(w) {}
};
std::vector<State> rk4(double rs, double h, const State &state0);