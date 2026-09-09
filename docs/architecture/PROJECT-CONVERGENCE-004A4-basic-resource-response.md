# PROJECT-CONVERGENCE-004A4 - Basic Resource Stress Response

## Purpose

004A4 extends the verified causal chain:

    environmental shock
        -> agricultural output loss
        -> food scarcity
        -> reduced POP life-needs fulfillment
        -> lagged survival-needs stress
        -> health vulnerability pressure
        -> migration push pressure
        -> resource-related instability pressure

Food is therefore no longer merely an economic commodity in the causal
simulation.

Failure to acquire enough basic food can propagate into health,
demographic, political, and eventually military systems.

004A4 deliberately produces pressures rather than irreversible outcomes.

It does not directly create:

- deaths;
- disease cases;
- migrants;
- refugees;
- riots;
- rebellion;
- civil war;
- interstate war.

## Real-world modeling rule

Empirical food-security analysis does not support a universal equation in
which a fixed food deficit automatically produces fixed numbers of deaths,
migrants, or conflicts.

The downstream pathways have different mechanisms and moderators.

Therefore 004A4 exposes typed causal inputs to those future domain models.

## Health and malnutrition

IPC acute-malnutrition analysis treats inadequate dietary intake and
disease or health conditions as interacting immediate causes of acute
malnutrition.

The causal structure is approximately:

    inadequate dietary intake
        + infectious disease
        + WASH conditions
        + health-care availability
        -> acute malnutrition
        -> morbidity and elevated mortality risk

WHO similarly identifies wasting as commonly resulting from inadequate
food quantity or quality and/or infectious disease.

004A4 therefore exposes:

    health_vulnerability_pressure =
        survival_needs_stress

This represents only the nutritional side of health vulnerability.

It is not itself:

- malnutrition prevalence;
- disease prevalence;
- morbidity;
- mortality probability.

A later health model should combine it with:

- age and demographic structure;
- disease prevalence;
- pregnancy;
- water and sanitation;
- vaccination;
- medical access;
- treatment capacity;
- baseline morbidity.

References:

- IPC Acute Food Insecurity analytical framework:
  https://www.ipcinfo.org/ipc-manual-interactive/ipc-acute-food-insecurity-protocols/function-2-classify-severity-and-identify-key-drivers/protocol-21-converge-evidence-using-the-ipc-analytical-framework/en/

- WHO Malnutrition:
  https://www.who.int/news-room/fact-sheets/detail/malnutrition/

## Migration and displacement

Food insecurity can increase the incentive to leave a location.

However, severe deprivation can also destroy the money, assets, health,
transport access, and other resources required to move.

The simulation therefore must distinguish:

    desire to move

from:

    ability to move.

004A4 exposes:

    mobility_push_pressure =
        survival_needs_stress

This is not migration itself.

OpenVic's existing migration-result fields remain authoritative and are
not changed by 004A4.

A future migration model should evaluate destination utility.

For origin i and destination j:

    V_ij =
        expected_income_j
        + employment_opportunity_j
        + food_security_j
        + physical_safety_j
        + social_network_support_ij
        - distance_cost_ij
        - transport_cost_ij
        - legal_or_border_cost_ij

A random-utility or multinomial-logit model can then use:

    P(i -> j) =
        exp(V_ij)
        / sum_k exp(V_ik)

subject to the population having enough mobility capacity to act on the
preference.

That permits both:

    food stress -> migration

and:

    severe deprivation -> trapped population.

Relevant modeling literature includes IOM work on food insecurity and
human mobility and World Bank analysis of food-security shocks and
migration.

## Instability, conflict, and war

Conflict is itself a major cause of hunger.

Food insecurity can also feed back into instability through:

- livelihood destruction;
- grievance;
- competition for scarce resources;
- recruitment opportunities;
- displacement;
- reduced institutional legitimacy.

But food scarcity alone is not an adequate conflict-onset model.

004A4 uses existing native POP militancy as the first available
susceptibility signal:

    instability_susceptibility =
        clamp(militancy / 10, 0, 1)

Resource-related instability pressure is:

    instability_pressure =
        survival_needs_stress
        * instability_susceptibility

Thus:

    high food stress + low native militancy
        -> low resource-instability pressure

while:

    high food stress + high native militancy
        -> greater resource-instability pressure.

This remains a pressure rather than probability of rebellion or war.

A future conflict hazard should combine it with variables such as:

- state capacity;
- income;
- inequality;
- repression;
- political exclusion;
- armed-group opportunity;
- security-force capability;
- prior conflict;
- foreign support;
- geographic conditions.

A future empirical statistical form could be:

    logit P(conflict onset) =
        beta_0
        + beta_resource * resource_instability
        + beta_income * income
        + beta_state * state_capacity
        + beta_inequality * inequality
        + beta_repression * repression
        + beta_armed * armed_opportunity
        + beta_history * conflict_history
        + ...

with coefficients calibrated against conflict data rather than invented
as game constants.

References:

- WFP Conflict and Hunger:
  https://www.wfp.org/conflict-and-hunger

- FAO Food Insecurity and Conflict:
  https://www.fao.org/4/y7352e/y7352e05.htm

## Shared exposure versus domain-specific outcomes

Health vulnerability and migration push initially equal the same
survival-needs stress value.

This does not imply that health and migration have identical empirical
elasticities.

They simply share the same upstream exposure.

Later models transform it differently:

    survival stress
        -> nutritional exposure
            x disease
            x demographics
            x WASH
            x health system
        -> health outcome

    survival stress
        -> mobility push
            + destination utility
            + mobility capacity
        -> migration outcome

    survival stress
        x instability susceptibility
            + institutions
            + political opportunity
            + security conditions
        -> conflict hazard

## Derived-state rule

`BasicResourceStressResponse` is not stored as additional POP state.

It is calculated on demand from:

- authoritative `survival_needs_stress_exposure`;
- authoritative native POP militancy.

This avoids shadow state and guarantees that the result reflects current
militancy.

The response requires no independent serialization.

Only the history-dependent 004A3 survival-needs exposure must eventually
be persisted.

## Native authority

OpenVic remains authoritative for:

- POP life-needs fulfillment;
- POP militancy;
- POP migration-result accounting.

004A4 creates no second:

- food ledger;
- health ledger;
- migration ledger;
- rebellion ledger;
- war state.

## Scope boundary

004A4 does not implement:

- malnutrition prevalence;
- disease;
- mortality;
- fertility;
- actual migration;
- destination choice;
- refugee or displacement accounting;
- riots;
- rebellion;
- coups;
- civil war;
- interstate war;
- humanitarian relief;
- government food assistance.

Those require their own domain-specific, empirically calibrated models.
