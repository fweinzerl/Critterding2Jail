# Config

## Keybinds

### File
- `config/keybinds.default.json`
- optional local override: `config/keybinds.local.json`

### Notes
- One global keybind profile shared by:
  - `app_critterding`
  - `app_critterding_threads`
- This file is a proposal/contract for upcoming runtime loading.
- Runtime now loads keybind config in this order:
  1. `config/keybinds.default.json` (or `../config/...`)
  2. `config/keybinds.local.json` (or `../config/...`) as override
- `keybinds.local.json` is gitignored for per-developer custom bindings.