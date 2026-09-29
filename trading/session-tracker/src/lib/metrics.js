export function sumPnl(trades) {
  return trades.reduce((acc, t) => acc + t.pnl, 0);
}

export function losingCount(trades) {
  return trades.filter((t) => t.pnl < 0).length;
}

export function winRate(trades) {
  if (trades.length === 0) return null;
  const wins = trades.filter((t) => t.pnl > 0).length;
  return wins / trades.length;
}

export function bestTrade(trades) {
  if (trades.length === 0) return null;
  return trades.reduce((b, t) => (t.pnl > b.pnl ? t : b));
}

export function worstTrade(trades) {
  if (trades.length === 0) return null;
  return trades.reduce((w, t) => (t.pnl < w.pnl ? t : w));
}

export function lossLimitPct(trades, lossLimit) {
  if (!lossLimit || lossLimit <= 0) return 0;
  const consumed = Math.max(0, -sumPnl(trades));
  return consumed / lossLimit;
}

export function gainTargetPct(trades, gainTarget) {
  if (!gainTarget || gainTarget <= 0) return 0;
  const gain = Math.max(0, sumPnl(trades));
  return gain / gainTarget;
}

export function stopState(trades, params) {
  const reasons = [];
  const lossPct = lossLimitPct(trades, params.lossLimit);
  if (lossPct >= 1) reasons.push("lossLimit");
  if (params.maxTrades > 0 && trades.length >= params.maxTrades) reasons.push("maxTrades");
  if (params.maxLossTrades > 0 && losingCount(trades) >= params.maxLossTrades)
    reasons.push("maxLossTrades");
  const stopped = reasons.length > 0;
  return {
    stopped,
    reasons,
    lossWarning: !reasons.includes("lossLimit") && lossPct >= 0.8,
  };
}

export function formatDuration(ms) {
  const total = Math.max(0, Math.floor(ms / 1000));
  const h = Math.floor(total / 3600);
  const m = Math.floor((total % 3600) / 60);
  const s = total % 60;
  const pad = (n) => String(n).padStart(2, "0");
  return `${pad(h)}:${pad(m)}:${pad(s)}`;
}

export function computePnl({ size, entry, exit, side }) {
  const empty = (v) => v === null || v === undefined || v === "";
  if (empty(size) || empty(entry) || empty(exit)) return null;
  const s = Number(size), e = Number(entry), x = Number(exit);
  if (Number.isNaN(s) || Number.isNaN(e) || Number.isNaN(x)) return null;
  return s * (x - e) * (side === "short" ? -1 : 1);
}
