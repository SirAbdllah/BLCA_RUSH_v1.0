# BLCA RUSH — English guide

**Author: BLCA Development Team · License: MIT**

## Package

A standalone RUSH game mode requiring only vanilla Arma Reforger. The included Arland scenario has four sectors and native US/USSR teams. Each sector has A/B M-COM terminals. Attackers arm them, defenders defuse them, and destroying both advances the battle. Source modification and redistribution are allowed under `LICENSE`.

| System | Behavior |
|---|---|
| Start | At least two players, with a participant on each team, followed by warmup. |
| Objectives | Default 4-second arm, 5-second defuse, and 30-second detonation. |
| Victory | Attackers destroy every objective. Defenders win when time expires with no active charge or attacker tickets run out. |
| Overtime | An active charge keeps the round alive after the clock reaches zero. |
| Tickets | Attackers start with 100. First deployment is free; redeployment consumes a ticket. Defenders have unlimited tickets in this version. |
| Progression | Both objectives open the next sector and move deployment points. Normal sectors receive at least 30 minutes; the final sector receives 15–20 minutes depending on remaining time, preserving a larger existing clock. |
| Defender deployment | Main point plus A/B points; objective deployment searches for a safe position approximately 100 metres from the terminal. |
| Squad deployment | Follows a living squad member; blocked during combat or at unsafe positions. |
| Boundaries | Sector area and approach corridors, return warning, and enforcement after the deadline. Sector changes allow 120 seconds to retreat. |
| Spawn protection | 50-metre protected spawn areas. Boundary deaths permit a free redeploy. |
| Corpses | Server cleanup after 15 seconds by default, checking that the character is dead. |
| HUD | Tactical minimap, compass, squads, objective distances, ammunition, and sector boundaries. Local player red, squad green, other teammates blue. |
| Radio | Public/squad channels built from native game components. |

Third-party terrains, WCS, custom clothing/weapon/vehicle catalogues, the custom arsenal framework, news, and the administration suite are excluded. Native selection/deployment menus offer four vanilla roles per team: Assault, Engineer, Support, Recon. Native explosion and UI alerts remain; private music tracks are not included.

## Requirements and quick start

1. Install Arma Reforger and, for editing, Arma Reforger Tools.
2. Open the root `addon.gproj` in Workbench.
3. Open `Worlds/BLCA_Rush_Arland.ent` in World Editor and enter play mode.
4. Choose faction, native equipment, squad, and deployment point. Two opposing players are required for the normal match start.

The only declared dependency is vanilla `58D0FB3206B6F859`. No other Workshop add-on is needed. Keep all authored `.meta` files with the source.

```text
Project GUID: B1CA525553480001
Scenario: {94AA9386CDB35E4F}Missions/BLCA_Rush_Arland.conf
```

Load this source project and vanilla data for local development. After Workshop publication, use the actual published item's ID in the server mod list. `Examples/Server.example.json` is a template: adjust server identity, addresses, capacity, and publication ID. See the [official server configuration reference](https://community.bistudio.com/wiki/Arma_Reforger:Server_Config).

The core imposes no player-count ceiling; the host controls server capacity.

## Configuration

The authority creates `$profile:BLCARush/Rush.json` on first launch. Edit while the server is stopped, then restart. `Examples/Rush.example.json` supplies a complete starting configuration. Per-client settings are `$profile:BLCARush/HUD.json`; see `Examples/HUD.example.json`.

| Setting | Default | Meaning |
|---|---:|---|
| `MinimumPlayers` | 2 | Fixed minimum, including a player on each team. |
| `WarmupSeconds` | 20 | 0–120 seconds. |
| `AttackerTickets` | 100 | Attacker reinforcement budget, up to 999. |
| `RoundMinutes` | 45 | Initial clock, 5–180 minutes. |
| `PlantSeconds` / `DefuseSeconds` | 4 / 5 | Hold durations, 2–15 seconds. |
| `DetonationSeconds` | 30 | 10–120 seconds. |
| `TransitionSeconds` | 15 | Sector transition, 5–90 seconds. |
| `AttackerRespawnSeconds` / `DefenderRespawnSeconds` | 8 / 10 | 0–90 seconds. |
| `RetreatGraceSeconds` | 20 | Normal boundary grace; sector-change grace is 120 seconds in code. |
| `CorpseSeconds` | 15 | Corpse cleanup delay, 5–300 seconds. |
| `AutoAssignSquads` / `SquadSize` | true / 4 | Automatic grouping; squad size 4–6. |
| `BalanceTeams` | true | Balances faction selection. |
| `EnforceSpawnSafety` / `SpawnSafetyMeters` | true / 25 | Enemy-distance validation for spawning. |
| `KillScore` / `PlantScore` / `DefuseScore` | 100 / 200 / 200 | Individual action scores. |
| `DestroyScore` / `SectorScore` | 300 / 150 | Objective destruction and sector scores. |

HUD toggles are `Minimap`, `AudioAlerts`, and `SquadNames`. `MapHalfRange` is 50–100 metres; the grid is 25 metres. Arming/defusing use native interaction actions, including the controller input path.

## Administration

The in-game account must have the native server **Administrator** role. No external manager is needed. Use game chat commands:

```text
/rush status
/rush start
/rush stop
/rush tickets 100
/rush resetobjectives
/rush nextsector
```

`start` respects the player/team requirement. `stop` awards defenders the round. `tickets` changes the attacker balance before exhaustion is latched. `resetobjectives` resets surviving terminals and does not revive destroyed ones. `nextsector` advances when the round state permits. Results use the native game-end flow; restarting the dedicated-server process is a hosting policy, not a capability granted to clients by this add-on.

## Source layout

| Path | Responsibility |
|---|---|
| `Scripts/Game/BLCA_Rush/BLCA_RushGameModeComponent.c` | Authority, rounds, tickets, deployment, scoring, cleanup. |
| `BLCA_RushSettings.c` in the same directory | JSON settings, life/results data, ticket reservation rules. |
| `BLCA_RushObjectiveComponent.c` | M-COM interaction, RPC validation, detonation, damage. |
| `BLCA_RushSectorComponent.c` | Objective/spawn links and boundary geometry. |
| `BLCA_RushRespawn.c` | Spawn validation, reservation and rollback. |
| `BLCA_RushSquadSpawns.c`, `BLCA_RushCombatTracking.c` | Squad points and combat lock. |
| `BLCA_RushRules.c` | Team lock, squad assignment, admin commands. |
| `BLCA_RushHUDController.c`, `BLCA_RushMapTheme.c` | HUD, map, compass, boundaries. |
| `BLCA_RushVoice.c` | Native radio channels. |
| `Prefabs/BLCA_Rush/` | Mode, M-COM, squad point, radio definitions. |
| `Worlds/BLCA_Rush_Arland_Layers/Rush.layer` | Authored sector positions, objectives, points, native roles. |
| `UI/layouts/BLCA_Rush/` | Interface layouts. |
| `Assets/BLCA/` | Supplied logo source and runtime texture. |

## Editing, validating, publishing

Follow [map integration](MAP_INTEGRATION.md) for another terrain. Change native role equipment in the world's loadout manager. If you reference an external add-on, your integration scenario will require it; keep such integrations separate from this vanilla core.

Run `python Tools/validate_dependencies.py`, the `Validate BLCA RUSH` Workbench survey, and a staging dedicated-server test after relevant changes. Keep source, `.meta`, `LICENSE`, and `NOTICE.md`; exclude generated caches, profiles, logs, and credentials according to `.gitignore`.

Windows/Workbench testing does not establish an actual PlayStation/Xbox test. `Validation/REPORT.md` records completed checks and limits. Base-game resources are referenced, not redistributed; their ownership remains described in `NOTICE.md`.
