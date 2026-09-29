#!/usr/bin/env python3
import argparse
import json
from datetime import date
from pathlib import Path

DEFAULT_MAX_GROUP_SIZE = 5
DEFAULT_STALE_DAYS = 7


def normalize_symbol(symbol):
    return symbol.strip().upper()


def normalize_optional(value):
    if value is None:
        return ""
    return value.strip().upper()


def load_watchlist(path):
    if not path.exists():
        return {"version": 2, "entries": []}

    with path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)

    if isinstance(data, list):
        data = {"version": 2, "entries": data}

    if not isinstance(data, dict):
        raise ValueError("Watchlist file must contain a JSON object or array.")

    data["version"] = max(int(data.get("version", 1)), 2)
    data.setdefault("entries", [])
    if not isinstance(data["entries"], list):
        raise ValueError("Watchlist 'entries' must be a list.")

    for entry in data["entries"]:
        entry.setdefault("asset_type", "unknown")
        entry.setdefault("exchange", "")
        entry.setdefault("notes", "")

    return data


def save_watchlist(path, data):
    data["entries"] = sorted(data["entries"], key=lambda entry: entry["symbol"])
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        json.dump(data, handle, indent=2, ensure_ascii=False)
        handle.write("\n")


def find_entry(entries, symbol):
    for entry in entries:
        if entry.get("symbol") == symbol:
            return entry
    return None


def entry_age_days(entry, today=None):
    today = today or date.today()
    added_at = entry.get("added_at", "")
    try:
        return (today - date.fromisoformat(added_at)).days
    except ValueError:
        return None


def watchlist_health_lines(data, max_group_size, stale_days):
    entries = data["entries"]
    if not entries:
        return []

    lines = []
    type_counts = {}
    exchange_counts = {}

    for entry in entries:
        asset_type = entry.get("asset_type", "unknown") or "unknown"
        exchange = entry.get("exchange", "")
        type_counts[asset_type] = type_counts.get(asset_type, 0) + 1
        if exchange:
            exchange_counts[exchange] = exchange_counts.get(exchange, 0) + 1

    for asset_type, count in sorted(type_counts.items()):
        if count > max_group_size:
            lines.append(
                f"warning: asset_type={asset_type} has {count} entries "
                f"(limit={max_group_size}); consider cleaning it."
            )

    for exchange, count in sorted(exchange_counts.items()):
        if count > max_group_size:
            lines.append(
                f"warning: exchange={exchange} has {count} entries "
                f"(limit={max_group_size}); consider cleaning it."
            )

    stale_entries = []
    for entry in sorted(entries, key=lambda item: item.get("symbol", "")):
        age = entry_age_days(entry)
        if age is not None and age > stale_days:
            stale_entries.append(f"{entry.get('symbol', '')}({age}d)")

    if stale_entries:
        symbols = ", ".join(stale_entries)
        lines.append(
            f"warning: entries older than {stale_days} days: {symbols}. "
            "Review whether to keep or remove them."
        )

    return lines


def print_watchlist_health(data, args):
    for line in watchlist_health_lines(data, args.max_group_size, args.stale_days):
        print(line)


def entry_rows(entries):
    rows = []
    for entry in sorted(entries, key=lambda item: item.get("symbol", "")):
        rows.append(
            [
                entry.get("symbol", ""),
                entry.get("asset_type", "unknown"),
                entry.get("exchange", ""),
                entry.get("added_at", ""),
                entry.get("notes", ""),
            ]
        )
    return rows


def print_table(entries):
    headers = ["Symbol", "Type", "Exchange", "Added", "Notes"]
    rows = entry_rows(entries)
    widths = [len(header) for header in headers]

    for row in rows:
        for index, value in enumerate(row):
            widths[index] = max(widths[index], len(value))

    def format_row(row):
        return "| " + " | ".join(
            value.ljust(widths[index]) for index, value in enumerate(row)
        ) + " |"

    print(format_row(headers))
    print("| " + " | ".join("-" * width for width in widths) + " |")
    for row in rows:
        print(format_row(row))


def grouped_entries(entries, group_by):
    groups = {}
    for entry in entries:
        if group_by == "asset_type":
            key = entry.get("asset_type", "unknown") or "unknown"
        elif group_by == "exchange":
            key = entry.get("exchange", "") or "NO_EXCHANGE"
        else:
            key = "Watchlist"
        groups.setdefault(key, []).append(entry)
    return sorted(groups.items())


def add_entry(args):
    path = Path(args.file)
    data = load_watchlist(path)
    symbol = normalize_symbol(args.symbol)
    exchange = normalize_optional(args.exchange)
    added_at = args.date or date.today().isoformat()
    entry = find_entry(data["entries"], symbol)

    if entry is None:
        entry = {
            "symbol": symbol,
            "asset_type": args.asset_type,
            "exchange": exchange,
            "added_at": added_at,
            "notes": args.notes or "",
        }
        data["entries"].append(entry)
        action = "added"
    else:
        if args.asset_type != "unknown":
            entry["asset_type"] = args.asset_type
        if args.exchange is not None:
            entry["exchange"] = exchange
        if args.notes is not None:
            entry["notes"] = args.notes
        entry.setdefault("added_at", added_at)
        entry.setdefault("notes", "")
        entry.setdefault("asset_type", "unknown")
        entry.setdefault("exchange", "")
        action = "updated"

    save_watchlist(path, data)
    print(f"{action}: {symbol} added_at={entry['added_at']}")
    print_watchlist_health(data, args)


def remove_entry(args):
    path = Path(args.file)
    data = load_watchlist(path)
    symbol = normalize_symbol(args.symbol)
    before = len(data["entries"])
    data["entries"] = [
        entry for entry in data["entries"] if entry.get("symbol") != symbol
    ]

    if len(data["entries"]) == before:
        print(f"not_found: {symbol}")
        return

    save_watchlist(path, data)
    print(f"removed: {symbol}")


def list_entries(args):
    data = load_watchlist(Path(args.file))
    entries = data["entries"]
    if args.format == "table":
        if not entries:
            print("empty: watchlist has no entries.")
        for group, group_entries in grouped_entries(entries, args.group_by):
            if args.group_by != "none":
                print(f"\n## {group}")
            print_table(group_entries)
    else:
        for entry in sorted(entries, key=lambda item: item.get("symbol", "")):
            symbol = entry.get("symbol", "")
            asset_type = entry.get("asset_type", "unknown")
            exchange = entry.get("exchange", "")
            added_at = entry.get("added_at", "")
            notes = entry.get("notes", "")
            exchange_part = f" exchange={exchange}" if exchange else ""
            suffix = f" notes={notes}" if notes else ""
            print(
                f"{symbol} asset_type={asset_type}{exchange_part} "
                f"added_at={added_at}{suffix}"
            )
    print_watchlist_health(data, args)


def review_entries(args):
    data = load_watchlist(Path(args.file))
    lines = watchlist_health_lines(data, args.max_group_size, args.stale_days)
    if not lines:
        print("review_ok: watchlist is focused.")
        return
    for line in lines:
        print(line)


def add_review_arguments(command):
    command.add_argument(
        "--max-group-size",
        type=int,
        default=DEFAULT_MAX_GROUP_SIZE,
        help="Warn when an asset type or exchange has more entries than this.",
    )
    command.add_argument(
        "--stale-days",
        type=int,
        default=DEFAULT_STALE_DAYS,
        help="Warn when an entry is older than this many days.",
    )


def build_parser():
    parser = argparse.ArgumentParser(description="Manage a simple trading watchlist.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    add = subparsers.add_parser("add", help="Add or update an entry.")
    add.add_argument("symbol")
    add.add_argument("--file", default="watchlist.json", help="Watchlist JSON path.")
    add.add_argument("--asset-type", default="unknown")
    add.add_argument("--exchange", help="Stock exchange such as NASDAQ, NYSE, or EPA.")
    add.add_argument("--notes")
    add.add_argument("--date", help="Override added date in YYYY-MM-DD format.")
    add_review_arguments(add)
    add.set_defaults(func=add_entry)

    remove = subparsers.add_parser("remove", help="Remove an entry.")
    remove.add_argument("symbol")
    remove.add_argument("--file", default="watchlist.json", help="Watchlist JSON path.")
    remove.set_defaults(func=remove_entry)

    list_command = subparsers.add_parser("list", help="List entries.")
    list_command.add_argument("--file", default="watchlist.json", help="Watchlist JSON path.")
    list_command.add_argument(
        "--format",
        choices=["plain", "table"],
        default="plain",
        help="Output format.",
    )
    list_command.add_argument(
        "--group-by",
        choices=["none", "asset_type", "exchange"],
        default="none",
        help="Group table output by asset type or exchange.",
    )
    add_review_arguments(list_command)
    list_command.set_defaults(func=list_entries)

    review = subparsers.add_parser("review", help="Review watchlist focus.")
    review.add_argument("--file", default="watchlist.json", help="Watchlist JSON path.")
    add_review_arguments(review)
    review.set_defaults(func=review_entries)

    return parser


def main():
    parser = build_parser()
    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
