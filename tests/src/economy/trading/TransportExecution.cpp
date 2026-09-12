#include "snitch/snitch.hpp"

#include <array>
#include <vector>

#include "openvic-simulation/economy/trading/TransportExecution.hpp"

using namespace OpenVic;

namespace {

TransportExecutionState make_state() {
    TransportExecutionState state;

    REQUIRE(
        state.configure_resources({
            TransportExecutionResource {
                .resource_id = "lift",
                .nominal_capacity = fixed_point_t { 10 }
            },
            TransportExecutionResource {
                .resource_id = "drivers",
                .nominal_capacity = fixed_point_t { 4 }
            },
            TransportExecutionResource {
                .resource_id = "handling",
                .nominal_capacity = fixed_point_t { 3 }
            }
        })
    );

    return state;
}

}

TEST_CASE(
    "005A24 reservation occupies scarce capacity until release date",
    "[convergence][005a24][logistics][transport][occupancy]"
) {
    auto state = make_state();

    std::array requirements {
        TransportExecutionRequirement {
            .resource_id = "lift",
            .required_capacity = fixed_point_t { 6 }
        }
    };

    unique_id_t reservation_id = 0;

    REQUIRE(
        state.reserve(
            1,
            requirements,
            Date { 2026, 1, 1 },
            Timespan { 3 },
            &reservation_id
        )
    );

    CHECK(reservation_id == 1);
    CHECK(
        state.get_reserved_capacity("lift") ==
        fixed_point_t { 6 }
    );
    CHECK(
        state.get_available_capacity("lift") ==
        fixed_point_t { 4 }
    );

    state.advance_to(Date { 2026, 1, 3 });
    CHECK(
        state.get_available_capacity("lift") ==
        fixed_point_t { 4 }
    );

    state.advance_to(Date { 2026, 1, 4 });
    CHECK(
        state.get_available_capacity("lift") ==
        fixed_point_t { 10 }
    );
}

TEST_CASE(
    "005A24 longer trip ties up the same transport resource longer",
    "[convergence][005a24][logistics][transport][distance]"
) {
    auto state = make_state();

    std::array full_lift {
        TransportExecutionRequirement {
            .resource_id = "lift",
            .required_capacity = fixed_point_t { 10 }
        }
    };

    REQUIRE(
        state.reserve(
            1,
            full_lift,
            Date { 2026, 2, 1 },
            Timespan { 8 }
        )
    );

    CHECK(
        state.get_available_capacity("lift") ==
        fixed_point_t::_0
    );

    state.advance_to(Date { 2026, 2, 5 });
    CHECK(
        state.get_available_capacity("lift") ==
        fixed_point_t::_0
    );

    state.advance_to(Date { 2026, 2, 9 });
    CHECK(
        state.get_available_capacity("lift") ==
        fixed_point_t { 10 }
    );
}

TEST_CASE(
    "005A24 multiple movements share finite transport capacity",
    "[convergence][005a24][logistics][transport][sharing]"
) {
    auto state = make_state();

    std::array first {
        TransportExecutionRequirement {
            .resource_id = "lift",
            .required_capacity = fixed_point_t { 4 }
        }
    };

    std::array second {
        TransportExecutionRequirement {
            .resource_id = "lift",
            .required_capacity = fixed_point_t { 6 }
        }
    };

    std::array excess {
        TransportExecutionRequirement {
            .resource_id = "lift",
            .required_capacity = fixed_point_t { 1 }
        }
    };

    REQUIRE(
        state.reserve(
            1, first,
            Date { 2026, 3, 1 },
            Timespan { 4 }
        )
    );

    REQUIRE(
        state.reserve(
            2, second,
            Date { 2026, 3, 1 },
            Timespan { 2 }
        )
    );

    CHECK_FALSE(
        state.reserve(
            3, excess,
            Date { 2026, 3, 1 },
            Timespan { 1 }
        )
    );

    CHECK(
        state.get_reserved_capacity("lift") ==
        fixed_point_t { 10 }
    );
}

TEST_CASE(
    "005A24 multi-resource reservation fails atomically",
    "[convergence][005a24][logistics][transport][atomic]"
) {
    auto state = make_state();

    std::array requirements {
        TransportExecutionRequirement {
            .resource_id = "lift",
            .required_capacity = fixed_point_t { 5 }
        },
        TransportExecutionRequirement {
            .resource_id = "drivers",
            .required_capacity = fixed_point_t { 5 }
        }
    };

    CHECK_FALSE(
        state.reserve(
            1,
            requirements,
            Date { 2026, 4, 1 },
            Timespan { 2 }
        )
    );

    CHECK(
        state.get_reserved_capacity("lift") ==
        fixed_point_t::_0
    );
    CHECK(
        state.get_reserved_capacity("drivers") ==
        fixed_point_t::_0
    );
    CHECK(state.get_reservations().empty());
}

TEST_CASE(
    "005A24 availability changes usable execution capacity independently of reservations",
    "[convergence][005a24][logistics][transport][availability]"
) {
    auto state = make_state();

    REQUIRE(
        state.set_resource_availability(
            "lift",
            fixed_point_t { 1 } / 2
        )
    );

    CHECK(
        state.get_effective_capacity("lift") ==
        fixed_point_t { 5 }
    );

    std::array requirements {
        TransportExecutionRequirement {
            .resource_id = "lift",
            .required_capacity = fixed_point_t { 5 }
        }
    };

    REQUIRE(
        state.reserve(
            1,
            requirements,
            Date { 2026, 5, 1 },
            Timespan { 2 }
        )
    );

    CHECK(
        state.get_available_capacity("lift") ==
        fixed_point_t::_0
    );

    REQUIRE(
        state.set_resource_availability(
            "lift",
            fixed_point_t { 3 } / 4
        )
    );

    CHECK(
        state.get_available_capacity("lift") ==
        fixed_point_t { 5 } / 2
    );

    state.advance_to(Date { 2026, 5, 3 });

    CHECK(
        state.get_available_capacity("lift") ==
        fixed_point_t { 15 } / 2
    );
}

TEST_CASE(
    "005A24 malformed execution requirements do not reserve capacity",
    "[convergence][005a24][logistics][transport][validation]"
) {
    auto state = make_state();

    std::array duplicate {
        TransportExecutionRequirement {
            .resource_id = "lift",
            .required_capacity = fixed_point_t { 1 }
        },
        TransportExecutionRequirement {
            .resource_id = "lift",
            .required_capacity = fixed_point_t { 1 }
        }
    };

    CHECK_FALSE(
        state.reserve(
            1,
            duplicate,
            Date { 2026, 6, 1 },
            Timespan { 1 }
        )
    );

    std::array unknown {
        TransportExecutionRequirement {
            .resource_id = "nonexistent",
            .required_capacity = fixed_point_t { 1 }
        }
    };

    CHECK_FALSE(
        state.reserve(
            2,
            unknown,
            Date { 2026, 6, 1 },
            Timespan { 1 }
        )
    );

    CHECK(state.get_reservations().empty());
}
