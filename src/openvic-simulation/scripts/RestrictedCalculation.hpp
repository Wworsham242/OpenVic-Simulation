#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

enum class RestrictedCalculationOperation : uint8_t {
    PUSH_INPUT,
    PUSH_CONSTANT,
    ADD,
    SUBTRACT,
    MULTIPLY,
    MINIMUM,
    MAXIMUM,
    NEGATE
};

struct RestrictedCalculationInstruction final {
    RestrictedCalculationOperation operation =
        RestrictedCalculationOperation::PUSH_CONSTANT;

    uint8_t input_index = 0;
    fixed_point_t constant = fixed_point_t::_0;

    bool operator==(
        RestrictedCalculationInstruction const&
    ) const = default;
};

class RestrictedCalculationDefinition final {
public:
    static constexpr uint8_t MAX_INPUTS = 8;
    static constexpr size_t MAX_INSTRUCTIONS = 16;
    static constexpr size_t MAX_STACK_DEPTH = 8;

private:
    uint8_t input_count = 0;
    memory::vector<RestrictedCalculationInstruction> instructions;

public:
    RestrictedCalculationDefinition() = default;

    RestrictedCalculationDefinition(
        uint8_t new_input_count,
        memory::vector<RestrictedCalculationInstruction> new_instructions
    ) :
        input_count { new_input_count },
        instructions { std::move(new_instructions) } {}

    [[nodiscard]] uint8_t get_input_count() const {
        return input_count;
    }

    [[nodiscard]] std::span<
        RestrictedCalculationInstruction const
    > get_instructions() const {
        return instructions;
    }

    [[nodiscard]] bool is_valid() const {
        if (
            input_count == 0 ||
            input_count > MAX_INPUTS ||
            instructions.empty() ||
            instructions.size() > MAX_INSTRUCTIONS
        ) {
            return false;
        }

        size_t stack_depth = 0;

        for (auto const& instruction : instructions) {
            switch (instruction.operation) {
                case RestrictedCalculationOperation::PUSH_INPUT:
                    if (
                        instruction.input_index >= input_count ||
                        stack_depth >= MAX_STACK_DEPTH
                    ) {
                        return false;
                    }

                    ++stack_depth;
                    break;

                case RestrictedCalculationOperation::PUSH_CONSTANT:
                    if (stack_depth >= MAX_STACK_DEPTH) {
                        return false;
                    }

                    ++stack_depth;
                    break;

                case RestrictedCalculationOperation::ADD:
                case RestrictedCalculationOperation::SUBTRACT:
                case RestrictedCalculationOperation::MULTIPLY:
                case RestrictedCalculationOperation::MINIMUM:
                case RestrictedCalculationOperation::MAXIMUM:
                    if (stack_depth < 2) {
                        return false;
                    }

                    --stack_depth;
                    break;

                case RestrictedCalculationOperation::NEGATE:
                    if (stack_depth < 1) {
                        return false;
                    }

                    break;

                default:
                    return false;
            }
        }

        return stack_depth == 1;
    }

    [[nodiscard]] std::optional<fixed_point_t> evaluate(
        std::span<fixed_point_t const> inputs
    ) const {
        if (
            !is_valid() ||
            inputs.size() != input_count
        ) {
            return std::nullopt;
        }

        std::array<
            fixed_point_t,
            MAX_STACK_DEPTH
        > stack {};

        size_t stack_size = 0;

        auto push = [&](fixed_point_t value) {
            stack[stack_size++] = value;
        };

        auto pop = [&]() {
            return stack[--stack_size];
        };

        for (auto const& instruction : instructions) {
            switch (instruction.operation) {
                case RestrictedCalculationOperation::PUSH_INPUT:
                    push(inputs[instruction.input_index]);
                    break;

                case RestrictedCalculationOperation::PUSH_CONSTANT:
                    push(instruction.constant);
                    break;

                case RestrictedCalculationOperation::ADD: {
                    fixed_point_t const rhs = pop();
                    fixed_point_t const lhs = pop();
                    push(lhs + rhs);
                    break;
                }

                case RestrictedCalculationOperation::SUBTRACT: {
                    fixed_point_t const rhs = pop();
                    fixed_point_t const lhs = pop();
                    push(lhs - rhs);
                    break;
                }

                case RestrictedCalculationOperation::MULTIPLY: {
                    fixed_point_t const rhs = pop();
                    fixed_point_t const lhs = pop();
                    push(lhs * rhs);
                    break;
                }

                case RestrictedCalculationOperation::MINIMUM: {
                    fixed_point_t const rhs = pop();
                    fixed_point_t const lhs = pop();
                    push(std::min(lhs, rhs));
                    break;
                }

                case RestrictedCalculationOperation::MAXIMUM: {
                    fixed_point_t const rhs = pop();
                    fixed_point_t const lhs = pop();
                    push(std::max(lhs, rhs));
                    break;
                }

                case RestrictedCalculationOperation::NEGATE:
                    stack[stack_size - 1] =
                        -stack[stack_size - 1];
                    break;

                default:
                    return std::nullopt;
            }
        }

        return stack[0];
    }

    bool operator==(
        RestrictedCalculationDefinition const&
    ) const = default;
};

}
