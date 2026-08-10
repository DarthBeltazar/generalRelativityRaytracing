//
// Created by AlexandrGeorgiev on 05.08.2026.
//
#include <vector>
#include "RayIntegrator.h"


State State::operator+(const State &other) const {return State(u+other.u, w+other.w);}
State State::operator*(double d) const {return State(u*d, w*d);}

State f(const State &state, double rs) {
    return State(state.w, -state.u + 1.5 * rs * state.u * state.u);
}

State rk4Step(State y, double rs, double h) {
    State k1 = f(y, rs);
    State k2 = f(y + k1*(h/2), rs);
    State k3 = f(y + k2*(h/2), rs);
    State k4 = f(y + k3*h, rs);
    State y_next = y + (k1 + k2 + k2 + k3 + k3 + k4)*(h/6);
    return y_next;
}

std::vector<State> rk4(double rs, double h, const State &state0) {
    State y = state0;
    std::vector<State> output;
    output.reserve(1001);
    output.push_back(y);
    for (int i = 0; i < 1000; i++) {
        y = rk4Step(y, rs, h);
        output.push_back(y);
        if (y.u >= 1/rs || y.u <= 0) {
            break;
        }
    }
    return output;
}
