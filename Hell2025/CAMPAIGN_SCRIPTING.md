# Campaign scripting

Hell2025 now supports a lightweight data-driven campaign layer.

## Files

Campaign scripts live at:

```
res/campaigns/<mapName>.json
```

Starting `GameMode::CAMPAIGN` automatically looks for the script matching the loaded map. The current test campaign is:

```
res/campaigns/Shit.json
```

## Stage format

Each stage has an id, objective, completion condition, and optional actions.

```json
{
  "id": "find_key",
  "objective": "Find the key.",
  "condition": {
    "type": "inventory_has",
    "item": "SmallKey"
  },
  "onEnter": [],
  "onComplete": []
}
```

Supported conditions:

- `inventory_has`: completes when any local player owns the named item.
- `enter_volume`: completes when any living local player enters the AABB described by `min` and `max`.
- `flag_set`: completes when an internal campaign flag is true.
- `always`: completes on the next campaign update.

Supported actions:

- `message`: typewriter message. Optional `duration`.
- `set_flag`: set an internal flag. Fields: `flag`, `value`.
- `lock_door`: lock all openable nodes on the door whose EditorName matches `target`.
- `unlock_door`: unlock all openable nodes on the named door.
- `play_audio`: play an existing audio asset by filename.
- `spawn_pickup`: spawn an existing Bible item. Fields: `item`, `position`, optional `rotation`.
- `spawn_enemy`: spawn `Dobermann`, `Kangaroo`, `Snake`, or `Shark`. Fields: `enemy`, `position`, optional `rotation`, optional `target` as the spawned editor name.
- `load_map`: request another campaign map. Field: `target`.

## Editor naming

Door actions use the existing **EditorName** field. The included test campaign currently targets TinyHouse's existing door names `Door` and `Door 6`. If a named door is not present, the campaign logs a warning and continues.

## Included test flow

The starter THE HOUSE campaign is wired directly to the current `Shit.map` + `TinyHouse.house` content:

1. A SmallKeySilver is spawned outside and the existing front door (`Door`) is locked.
2. Picking up the silver key unlocks the front door.
3. The existing SmallKey inside TinyHouse unlocks the existing `Door 6`.
4. A BlackSkull is spawned behind that locked area.
5. The existing Remington870 becomes the next objective.
6. Taking it spawns an exit Dobermann.
7. A P90 is spawned at the escape cache; collecting it completes the chapter.

The supplied coordinates are a first-pass script against the current map data and are intentionally easy to tune in `res/campaigns/Shit.json`.

## Extending it

The campaign manager is intentionally small. The next natural action types are enemy spawning, light groups, map transitions, checkpoints/save state, and scripted object enable/disable.


### Volume trigger example

You can use a spatial trigger in later stages without adding a new map object:

```json
{
  "type": "enter_volume",
  "min": [10.0, 0.0, 10.0],
  "max": [14.0, 4.0, 14.0]
}
```

The condition succeeds when any living local player enters that world-space box.


## Spawn example

```json
{
  "type": "spawn_enemy",
  "enemy": "Dobermann",
  "target": "basement_hound",
  "position": [34.0, 32.0, 38.0],
  "rotation": [0.0, 1.57, 0.0]
}
```

```json
{
  "type": "spawn_pickup",
  "item": "SmallKeySilver",
  "position": [41.0, 33.0, 35.0],
  "rotation": [0.0, 0.0, 0.0]
}
```

A map transition can be attached to a stage's `onComplete`:

```json
{
  "type": "load_map",
  "target": "ChapterTwo"
}
```
