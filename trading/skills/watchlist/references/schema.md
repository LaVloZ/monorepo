# Watchlist Schema

Use a single JSON file with this shape:

```json
{
  "version": 2,
  "entries": [
    {
      "symbol": "BTC",
      "asset_type": "crypto",
      "exchange": "",
      "added_at": "2026-05-30",
      "notes": "Optional note"
    },
    {
      "symbol": "NVDA",
      "asset_type": "stock",
      "exchange": "NASDAQ",
      "added_at": "2026-05-30",
      "notes": ""
    }
  ]
}
```

Field rules:

- `version`: integer schema version. Start with `2`.
- `entries`: array of watchlist entries.
- `symbol`: required string, normalized uppercase, unique in the file.
- `asset_type`: string, default `unknown`. Use `crypto` or `stock` for now.
- `exchange`: string, default empty string. Use mainly for stock entries, such as `NASDAQ`, `NYSE`, `AMEX`, `EPA`, `LSE`, or `XETRA`.
- `added_at`: required local date in `YYYY-MM-DD`.
- `notes`: string, default empty string.

Accept older/simple files where the root is directly an array of entries, but write back using the object shape above.
