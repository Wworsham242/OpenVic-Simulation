#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/military/MilitaryFormationInstance.hpp"
#include "openvic-simulation/types/UniqueId.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

/*
 * Aggregate consumable holding for one formation and one
 * data-defined sustainment item.
 *
 * One record may represent large physical quantities.
 * It is not an individual round, container, meal, fuel can,
 * spare part, or other microscopic object.
 */
struct MilitarySustainmentStock {
    unique_id_t formation_unique_id = 0;
    std::string item_id;

    fixed_point_t target_quantity = fixed_point_t::_0;
    fixed_point_t storage_capacity = fixed_point_t::_0;
    fixed_point_t current_quantity = fixed_point_t::_0;
};

class MilitarySustainmentStockState final {
private:
    std::vector<MilitarySustainmentStock> stocks;

    MilitarySustainmentStock* find_stock(
        unique_id_t formation_unique_id,
        std::string_view item_id
    );

    MilitarySustainmentStock const* find_stock(
        unique_id_t formation_unique_id,
        std::string_view item_id
    ) const;

public:
    [[nodiscard]]
    std::span<MilitarySustainmentStock const>
    get_stocks() const {
        return stocks;
    }

    /*
     * Register one aggregate sustainment holding.
     *
     * target_quantity is the desired operating stock.
     * storage_capacity is the hard physical upper bound.
     */
    bool add_stock(
        MilitaryFormationInstanceManager const& formation_manager,
        unique_id_t formation_unique_id,
        std::string_view item_id,
        fixed_point_t target_quantity,
        fixed_point_t storage_capacity,
        fixed_point_t initial_quantity = fixed_point_t::_0
    );

    [[nodiscard]]
    fixed_point_t get_quantity(
        unique_id_t formation_unique_id,
        std::string_view item_id
    ) const;

    [[nodiscard]]
    fixed_point_t get_target_shortfall(
        unique_id_t formation_unique_id,
        std::string_view item_id
    ) const;

    [[nodiscard]]
    fixed_point_t get_remaining_capacity(
        unique_id_t formation_unique_id,
        std::string_view item_id
    ) const;

    /*
     * Receive up to available storage capacity.
     *
     * accepted_quantity reports the physical amount transferred
     * into this holding.
     */
    bool receive(
        unique_id_t formation_unique_id,
        std::string_view item_id,
        fixed_point_t offered_quantity,
        fixed_point_t& accepted_quantity
    );

    /*
     * Consume up to the quantity physically on hand.
     *
     * consumed_quantity may be less than requested_quantity.
     * This allows later activity mechanics to observe shortages
     * instead of inventing negative stock.
     */
    bool consume(
        unique_id_t formation_unique_id,
        std::string_view item_id,
        fixed_point_t requested_quantity,
        fixed_point_t& consumed_quantity
    );
};

}
