---
name: market-analysis
description: Analyze a stock, crypto, index, sector, or macro setup with a trader mindset. Use when Codex needs to study price action, market structure, support and resistance, trend strength, catalysts, news impact, macro context, or bullish/bearish scenarios; when a user asks for technical analysis, a market read, a trading plan, risk framing, or a simple educational explanation of why a move is happening.
---

# Market Analysis

Provide a clear, decision-oriented market read. Stay neutral, separate fact from inference, and explain the setup simply before adding nuance.

## Workflow

1. Identify the asset, timeframe, and user intent.
2. Build the current context:
   - Determine whether the user wants a quick read, a trade setup, a macro explanation, or teaching.
   - If the request depends on recent events, browse for current news, rates, tariffs, earnings, guidance, regulation, or policy decisions before concluding.
3. Read price action:
   - Trend direction
   - Market structure: higher highs/higher lows or lower highs/lower lows
   - Key support and resistance
   - Breakout, breakdown, range, rejection, or retest behavior
   - Momentum and volume if data is available
4. Connect catalysts to price:
   - State what is confirmed
   - State what is likely interpretation
   - Note when price is moving on positioning, liquidity, or sentiment rather than a hard catalyst
5. Turn the read into scenarios:
   - Bullish path
   - Bearish path
   - Invalidations and key levels
6. Match the explanation depth to the user:
   - Keep it simple by default
   - Add more detail only if the user asks

## Analysis Rules

- Do not force a bullish or bearish view.
- Do not present guesses as facts.
- Prefer concrete levels, dates, and catalysts over vague wording.
- Distinguish short-term trading context from higher-timeframe trend.
- If the move is news-driven, explain why that news matters for liquidity, rates, margins, growth, regulation, or risk appetite.
- If the user asks for a live or latest read, verify with current sources first.
- If confidence is low because data is missing, say so directly.

## Output Shape

Use this structure when helpful:

```md
## Context
- Asset:
- Timeframe:
- Current backdrop:

## Technical Read
- Trend:
- Structure:
- Key support:
- Key resistance:
- Momentum note:

## Catalysts
- Confirmed:
- Possible interpretation:

## Scenarios
- Bullish scenario:
- Bearish scenario:
- Invalidation:

## Bottom Line
- One concise conclusion in plain language
```

Keep the final answer concise unless the user requests a deeper breakdown.

## Reference

For a reusable checklist and teaching prompts, read [references/analysis-framework.md](references/analysis-framework.md).
