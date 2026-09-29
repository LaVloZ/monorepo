---
name: watchlist
description: Manage a simple local trading watchlist stored in a standard JSON file. Use when Codex needs to add entries to a watchlist, remove entries, list existing watchlist items, preserve the date each asset was added, distinguish crypto from stock assets, track stock exchanges, warn when the watchlist gets too large or stale, or maintain a focused lightweight watchlist file for market symbols.
---

# Watchlist

## Overview

Use this skill to maintain a lightweight trading watchlist without a database. Store entries in a JSON file, preserve the add date, and keep operations predictable.

Default to `watchlist.json` in the current working directory unless the user gives another path.

The user should be able to speak naturally to Codex, such as "ajoute BTC a ma watchlist" or "supprime TSLA de ma watchlist". Codex must translate that request into the appropriate `scripts/watchlist.py` command instead of asking the user to run the script manually.

The goal is focus: help the user keep only a few high-quality setups instead of accumulating too many unfocused positions.

## Data Model

Read `references/schema.md` before changing the file format.

Each entry must include:

- `symbol`: normalized uppercase ticker or market symbol.
- `asset_type`: category such as `crypto`, `stock`, or `unknown`.
- `exchange`: optional exchange code for stocks, such as `NASDAQ`, `NYSE`, `AMEX`, `EPA`, `LSE`, or `XETRA`.
- `added_at`: ISO date string in `YYYY-MM-DD` format.
- `notes`: optional free text.

Do not remove or overwrite `added_at` when an existing symbol is already present.

## Workflow

1. Identify the watchlist file path.
2. If the file does not exist, create it with an empty watchlist structure.
3. Use `scripts/watchlist.py` for add, remove, and list operations when possible.
4. After modifying the file, summarize the change in French with the symbol and date.
5. If the script prints warnings, explain them in French and suggest reviewing/removing stale or overcrowded groups.
6. Keep the format simple; avoid adding a database, API dependency, or complex folder structure unless the user asks.

## Commands

Add an entry:

```bash
python3 scripts/watchlist.py add BTC --asset-type crypto --file watchlist.json
```

Add an entry with notes:

```bash
python3 scripts/watchlist.py add NVDA --asset-type stock --exchange NASDAQ --notes "IA, momentum fort" --file watchlist.json
```

Remove an entry:

```bash
python3 scripts/watchlist.py remove BTC --file watchlist.json
```

List entries:

```bash
python3 scripts/watchlist.py list --file watchlist.json
```

List entries as one table:

```bash
python3 scripts/watchlist.py list --format table --file watchlist.json
```

List entries grouped by type:

```bash
python3 scripts/watchlist.py list --format table --group-by asset_type --file watchlist.json
```

List entries grouped by exchange:

```bash
python3 scripts/watchlist.py list --format table --group-by exchange --file watchlist.json
```

Review watchlist focus without listing every entry:

```bash
python3 scripts/watchlist.py review --file watchlist.json
```

## Behavior Rules

- Normalize symbols to uppercase.
- Normalize exchange codes to uppercase.
- Use `crypto` or `stock` for `asset_type` when the user gives enough context; otherwise use `unknown`.
- For stock entries, include `exchange` when the user specifies it or when it is obvious from the request.
- For crypto entries, leave `exchange` empty unless the user explicitly gives an exchange or venue.
- Treat duplicate additions as no-op updates: keep the original `added_at`; update `asset_type`, `exchange`, or `notes` only if explicitly provided.
- Sort entries alphabetically by `symbol` after each write.
- Use the local date for `added_at` unless the user gives a specific date.
- Warn when more than 5 entries exist in the same `asset_type` or same `exchange`.
- Warn when entries are older than 7 days so the user can decide whether to remove or keep them.
- Treat these warnings as focus prompts, not hard errors: do not remove entries unless the user asks.
- When the user asks to display the watchlist, prefer a readable table. Use `--group-by exchange` for stock-heavy watchlists, `--group-by asset_type` for mixed crypto/stock watchlists, and no grouping for short watchlists.
- If the user asks for analysis of watchlist assets, combine this skill with the market-analysis skill.
