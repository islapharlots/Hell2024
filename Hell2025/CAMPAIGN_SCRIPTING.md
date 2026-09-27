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

## Editor naming

Door actions use the existing **EditorName** field. For the included test campaign, name the relevant doors:

```
campaign_front_door
campaign_basement_door
```

If a named door is not present, the campaign logs a warning and continues.

## Included test flow

The starter THE HOUSE campaign uses existing engine items:

1. SmallKeySilver
2. SmallKey
3. BlackSkull
4. Remington870
5. Escape volume

Place these pickups where you want them with the existing editor. Adjust the final escape volume in `res/campaigns/Shit.json` after choosing the actual exit location.

## Extending it

The campaign manager is intentionally small. The next natural action types are enemy spawning, light groups, map transitions, checkpoints/save state, and scripted object enable/disable.
