import { describe, it, expect } from "vitest";
import { mount } from "@vue/test-utils";
import SessionDetail from "../src/components/SessionDetail.vue";

const session = { _id: "s1", startedAt: "2026-07-20T08:00:00.000Z", endedAt: "2026-07-20T09:00:00.000Z" };
const trades = [
  { _id: "t1", sessionId: "s1", ticker: "NQ", side: "long", pnl: 30, rMultiple: 1.5, note: "bon setup", tags: ["breakout"], createdAt: "2026-07-20T08:10:00.000Z", size: 2, entry: 100, exit: 115 },
  { _id: "t2", sessionId: "s1", ticker: "ES", side: "short", pnl: -10, rMultiple: null, note: "", tags: [], createdAt: "2026-07-20T08:30:00.000Z", size: null, entry: null, exit: null },
];

describe("SessionDetail", () => {
  it("affiche le P&L total, le nombre de trades et la durée", () => {
    const w = mount(SessionDetail, { props: { session, trades } });
    expect(w.text()).toContain("+20.00");
    expect(w.text()).toContain("2 trades");
    expect(w.text()).toContain("01:00:00");
  });
  it("liste chaque trade avec son ticker et son pnl", () => {
    const w = mount(SessionDetail, { props: { session, trades } });
    expect(w.text()).toContain("NQ");
    expect(w.text()).toContain("+30.00");
    expect(w.text()).toContain("ES");
    expect(w.text()).toContain("-10.00");
  });
  it("émet back au clic sur Retour", async () => {
    const w = mount(SessionDetail, { props: { session, trades } });
    await w.find(".back").trigger("click");
    expect(w.emitted("back")).toBeTruthy();
  });
  it("affiche 'Aucun trade' si la session est vide", () => {
    const w = mount(SessionDetail, { props: { session, trades: [] } });
    expect(w.text()).toContain("Aucun trade");
  });
});
