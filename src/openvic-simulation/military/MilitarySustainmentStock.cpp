#include "MilitarySustainmentStock.hpp"

#include <algorithm>

using namespace OpenVic;

MilitarySustainmentStock*
MilitarySustainmentStockState::
find_stock(
    unique_id_t formation_unique_id,
    std::string_view item_id
) {
    auto const it = std::find_if(
        stocks.begin(),
        stocks.end(),
        [formation_unique_id, item_id](auto const& stock) {
            return
                stock.formation_unique_id == formation_unique_id &&
                stock.item_id == item_id;
        }
    );

    return it != stocks.end() ? &*it : nullptr;
}

MilitarySustainmentStock const*
MilitarySustainmentStockState::
find_stock(
    unique_id_t formation_unique_id,
    std::string_view item_id
) const {
    auto const it = std::find_if(
        stocks.begin(),
        stocks.end(),
        [formation_unique_id, item_id](auto const& stock) {
            return
                stock.formation_unique_id == formation_unique_id &&
                stock.item_id == item_id;
        }
    );

    return it != stocks.end() ? &*it : nullptr;
}

bool
MilitarySustainmentStockState::
add_stock(
    MilitaryFormationInstanceManager const& formation_manager,
    unique_id_t formation_unique_id,
    std::string_view item_id,
    fixed_point_t target_quantity,
    fixed_point_t storage_capacity,
    fixed_point_t initial_quantity
) {
    if (
        formation_unique_id == 0 ||
        item_id.empty() ||
        target_quantity < fixed_point_t::_0 ||
        storage_capacity <= fixed_point_t::_0 ||
        target_quantity > storage_capacity ||
        initial_quantity < fixed_point_t::_0 ||
        initial_quantity > storage_capacity ||
        find_stock(formation_unique_id, item_id) != nullptr ||
        formation_manager.
            get_military_formation_instance_by_unique_id(
                formation_unique_id
            ) == nullptr
    ) {
        return false;
    }

    stocks.push_back(
        MilitarySustainmentStock {
            .formation_unique_id = formation_unique_id,
            .item_id = std::string { item_id },
            .target_quantity = target_quantity,
            .storage_capacity = storage_capacity,
            .current_quantity = initial_quantity
        }
    );

    return true;
}

fixed_point_t
MilitarySustainmentStockState::
get_quantity(
    unique_id_t formation_unique_id,
    std::string_view item_id
) const {
    auto const* stock =
        find_stock(formation_unique_id, item_id);

    return
        stock != nullptr
            ? stock->current_quantity
            : fixed_point_t::_0;
}

fixed_point_t
MilitarySustainmentStockState::
get_target_shortfall(
    unique_id_t formation_unique_id,
    std::string_view item_id
) const {
    auto const* stock =
        find_stock(formation_unique_id, item_id);

    if (stock == nullptr) {
        return fixed_point_t::_0;
    }

    return std::max(
        stock->target_quantity - stock->current_quantity,
        fixed_point_t::_0
    );
}

fixed_point_t
MilitarySustainmentStockState::
get_remaining_capacity(
    unique_id_t formation_unique_id,
    std::string_view item_id
) const {
    auto const* stock =
        find_stock(formation_unique_id, item_id);

    if (stock == nullptr) {
        return fixed_point_t::_0;
    }

    return std::max(
        stock->storage_capacity - stock->current_quantity,
        fixed_point_t::_0
    );
}

bool
MilitarySustainmentStockState::
receive(
    unique_id_t formation_unique_id,
    std::string_view item_id,
    fixed_point_t offered_quantity,
    fixed_point_t& accepted_quantity
) {
    accepted_quantity = fixed_point_t::_0;

    if (offered_quantity <= fixed_point_t::_0) {
        return false;
    }

    auto* stock =
        find_stock(formation_unique_id, item_id);

    if (stock == nullptr) {
        return false;
    }

    fixed_point_t const remaining =
        std::max(
            stock->storage_capacity - stock->current_quantity,
            fixed_point_t::_0
        );

    accepted_quantity =
        std::min(offered_quantity, remaining);

    stock->current_quantity += accepted_quantity;

    return true;
}

bool
MilitarySustainmentStockState::
consume(
    unique_id_t formation_unique_id,
    std::string_view item_id,
    fixed_point_t requested_quantity,
    fixed_point_t& consumed_quantity
) {
    consumed_quantity = fixed_point_t::_0;

    if (requested_quantity <= fixed_point_t::_0) {
        return false;
    }

    auto* stock =
        find_stock(formation_unique_id, item_id);

    if (stock == nullptr) {
        return false;
    }

    consumed_quantity =
        std::min(
            requested_quantity,
            stock->current_quantity
        );

    stock->current_quantity -= consumed_quantity;

    return true;
}
