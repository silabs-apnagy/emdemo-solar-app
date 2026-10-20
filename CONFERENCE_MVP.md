# Solar firmware — conference 2.0 preview

Checkpoint: **2026-10-20**. Feature scope is frozen; bug fixes/validation only.
Working branch: `SolarPanel2.0`. Behavior baseline: `12ce422`. The operator confirms
the currently running firmware represents the demo behavior below; that behavior
is now committed. Exact flashed artifact hashes are separate release records.

## Current demo behavior

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
  No flashing was done during bookkeeping. The operator confirms the running
  baseline; do not assume older copied/full-solution artifacts contain it.
- Keep the correct bootloader/application pair; no erase/recommission was performed.
- Validate ten-second energy totals, live power, mode reports, and RF subscription
  recovery during the release soak. Previous Pi inspection confirmed a solar subscription
  outage and high MAC retries; report throttling is mitigation, not proof of a fix.
- No global simulated clock, custom transport, or new cluster extension is added.

Full-system status and pinning handoff live in the `energy-management-demo`
repository's `docs/conference-status.md`.
