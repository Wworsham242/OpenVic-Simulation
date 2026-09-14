# PROJECT-CONVERGENCE-006B1.2 — Explicit Cohort Aging Flow

**Program:** PROJECT-CONVERGENCE-006B1 — Causal Population Evolution

## Purpose

Add deterministic age-band transition accounting without inventing an
unsupported within-band age distribution.

## Native Repository Basis

The engine currently represents demographic age using eighteen five-year
female/male age bands from age 0-4 through the open-ended age 85+ group.

The existing 004A6 architecture explicitly records that a five-year cohort
does not identify the exact ages of the people inside the band and warns that
moving one-fifth of each cohort annually would be an approximation rather than
an inherent consequence of the representation.

`SimTime` is scenario-agnostic and unitless. `Cadence` schedules periodic work
in ticks without assigning calendar meaning to those ticks.

Therefore demographic aging must not hard-code either:

- one-fifth annual aging; or
- a fixed number of simulation ticks per demographic year.

## External Reference Review

Cohort-component demographic methods advance population along age cohorts while
applying mortality, fertility, and migration separately.

Professional implementations vary their resolution. Modern Census and UN
methods can operate at single-year age and annual intervals, while five-year
age-group implementations can operate over five-year periods.

The engine therefore separates:

1. accounting for people who cross an age-band boundary; from
2. the model or dataset that determines how many cross that boundary during a
   particular demographic interval.

## Chosen Mechanism

`DemographicCohortAgingFlow` contains explicit female/male outflow counts for
each age band.

For each nonterminal age band:

    ending[i] =
        starting[i]
        - aging_outflow[i]
        + aging_outflow[i - 1]

Age 0-4 has no incoming aging flow.

Age 85+ is open-ended:

    ending[85+] =
        starting[85+]
        + aging_outflow[80-84]

No aging outflow from age 85+ is allowed.

All destination values are calculated from the original starting structure so
execution order cannot cause a person entering a cohort during one transition
to age again during that same transition.

## Calibration Status

No demographic aging rate is introduced.

All aging flows are explicit caller-supplied counts.

A future producer may derive those counts from:

- single-year age data;
- cohort reconstruction;
- an explicit five-year interval;
- or a documented approximation.

Any approximation must identify its calibration status and must not be hidden
inside this accounting substrate.

## Causal Integration

This mechanism preserves total population and female/male population
separately.

A later demographic-cycle increment will define ordering among:

- mortality;
- aging;
- births;
- migration.

B1.2 itself does not choose that full-cycle exposure convention.

## Scope Boundary

Not implemented here:

- automatic annual aging;
- one-fifth cohort approximation;
- within-band age reconstruction;
- mortality;
- fertility;
- births;
- migration;
- socioeconomic POP mutation;
- demographic scheduling;
- calendar-to-SimTime mapping;
- persistence integration.

## Acceptance

Tests must prove:

1. zero aging flow is identity;
2. explicit flow reaches exactly the adjacent older cohort;
3. simultaneous adjacent flows use original source stocks;
4. age 80-84 flows into age 85+;
5. age 85+ cannot age out;
6. outflow cannot exceed source population;
7. negative flow is rejected;
8. destination overflow is rejected atomically;
9. female/male totals are conserved independently;
10. identical inputs produce identical outputs.

Completion requires build, focused tests, full CTest, research gate, clean diff,
commit, push, and exact local/remote HEAD agreement.