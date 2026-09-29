import { sumPnl, winRate } from "./metrics.js";

export function sessionSummaries(sessions, tradesBySession) {
  return sessions.map((s) => {
    const ts = tradesBySession[s._id] || [];
    return {
      id: s._id,
      startedAt: s.startedAt,
      pnl: sumPnl(ts),
      count: ts.length,
      winRate: winRate(ts),
    };
  });
}

export function pnlByDay(sessions, trades) {
  const dayOf = {};
  // Regroupement par jour LOCAL (cohérent avec le calendrier, qui utilise l'heure locale).
  for (const s of sessions) dayOf[s._id] = new Date(s.startedAt).toLocaleDateString("en-CA");
  const map = new Map();
  for (const t of trades) {
    const day = dayOf[t.sessionId];
    if (!day) continue;
    map.set(day, (map.get(day) || 0) + t.pnl);
  }
  return map;
}
