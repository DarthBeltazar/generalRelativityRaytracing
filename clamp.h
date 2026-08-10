//
// Created by Александр Георгиев on 30.07.2026.
//
#pragma once

template<typename T>
T clamp(T value, T low, T high) {
    return value < low ? low : value > high ? high : value;
}