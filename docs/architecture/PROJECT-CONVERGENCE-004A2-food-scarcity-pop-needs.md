# PROJECT-CONVERGENCE-004A2 — Native Food Scarcity to POP Life-Needs Fulfillment

## Purpose

004A2 extends the 004A1 environmental/agricultural causal strand into
the existing native POP consumption system.

The intended causal path is:

    province environmental water availability
        -> farm RGO physical output
        -> native GoodMarket supply
        -> native POP BuyUpToOrder
        -> actual quantity acquired
        -> native POP life-needs fulfillment

004A2 deliberately does not introduce a separate food-security, hunger,
nutrition, famine, migration, mortality, or political-response system.

## Authoritative ownership

No new authoritative simulation state is required.

Existing systems remain authoritative:

- ProvinceEnvironmentalState owns environmental truth.
- ResourceGatheringOperation owns native agricultural physical production.
- GoodMarket owns market supply allocation and clearing.
- Pop owns desired/acquired needs and life-needs fulfillment.

The relevant native POP welfare signal is:

    life_needs_fulfilled = acquired life-needs quantity
                           / desired life-needs quantity

The purpose of 004A2 is to prove that upstream physical scarcity already
propagates into that native welfare state.

## Isolation of physical scarcity

The convergence test gives comparison POPs identical, deliberately
non-binding purchasing power.

This separates:

    environmental stress
        -> reduced physical food supply
        -> reduced acquired life needs

from the separate, also-valid future pathway:

    environmental stress
        -> farm revenue/wage effects
        -> household purchasing-power effects
        -> consumption effects

The latter should be tested separately rather than confounded with this
increment.

## Scope boundary

004A2 does not add:

- a FoodSecurityManager;
- a second hunger or welfare score;
- famine mechanics;
- mortality;
- migration;
- militancy;
- political unrest;
- relief policy;
- food stockpiles;
- calorie or nutrient classes;
- crop-specific agronomy;
- price controls.

If the end-to-end native test passes without simulation-code changes,
that is the preferred architectural result: the domains were already
causally connected and only lacked explicit convergence proof.
