# Solar firmware — conference 2.0 preview

Checkpoint: **2026-10-20**. Feature scope is frozen; bug fixes/validation only.
Working branch: `SolarPanel2.0`. Current committed base: `b524b26`; the features
below include uncommitted local changes and require a component commit before
updating the energy-management super-repo's solar pin.

## Current local behavior

- xG24 / BRD4187C Matter-over-Thread solar simulation.
- Endpoint 1 Mode Select: Dawn, Peak, Dusk, Night. Endpoint 2 Mode Select:
  Clear, Overcast. The second endpoint is a demo weather selector; no physical
  rain-sensor functionality is claimed.
- 4 kW nominal maximum; smooth one-minute profile ramps and timed cloud dips.
  Defaults are grouped near the beginning of `src/AppTask.cpp`.
- Dashboard drives modes; firmware owns local ramps, dips, output, and integration.
- Simulation and electrical power readings update every second. Periodic and
  cumulative exported energy publish every ten seconds, retaining fractional
  energy and the full interval total. Allocation/scheduling failures retain the
  pending interval for the next attempt.
- Matter UART logging is errors-only; hard-fault diagnostics remain. Full Thread
  logs were already disabled; dormant defaults are now warnings/no packet dumps.
  Configuration is recorded in both headers and `.slcp` for regeneration.

## Verification / next handoff

- App build succeeded on October 20 after the reliability changes.
- Updated app artifact: `MatterSolarPanelOverThread/artifact/MatterSolarPanelOverThread.s37`.
  This build was not flashed by the agent; do not assume older copied/full-solution
  artifacts contain it.
- Keep the correct bootloader/application pair; no erase/recommission was performed.
- Validate ten-second energy totals, live power, mode reports, and RF subscription
  recovery after flashing. Previous Pi inspection confirmed a solar subscription
  outage and high MAC retries; report throttling is mitigation, not proof of a fix.
- No global simulated clock, custom transport, or new cluster extension is added.

Full-system status and pinning handoff live in the `energy-management-demo`
repository's `docs/conference-status.md`.
