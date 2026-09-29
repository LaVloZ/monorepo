import { describe, it, expect, vi, beforeEach } from "vitest";
import { mount } from "@vue/test-utils";
import { setActivePinia, createPinia } from "pinia";

vi.mock("../src/db.js", () => ({
  createDoc: vi.fn(async () => ({ id: "x", rev: "1-x" })),
  updateDoc: vi.fn(), getDoc: vi.fn(), deleteDoc: vi.fn(), find: vi.fn(async () => []),
}));
import TradeForm from "../src/components/TradeForm.vue";
import { useSessionStore } from "../src/stores/session.js";

beforeEach(() => setActivePinia(createPinia()));

describe("TradeForm — garde-fou de saisie", () => {
  it("ne soumet pas quand le pnl a été saisi puis effacé (chaîne vide)", async () => {
    const wrapper = mount(TradeForm);
    const session = useSessionStore();
    const spy = vi.spyOn(session, "addTrade").mockResolvedValue();
    await wrapper.find('input[placeholder="Ticker (ex. NQ)"]').setValue("NQ");
    const pnl = wrapper.find('input[placeholder="Résultat (€/$)"]');
    await pnl.setValue("25");
    await pnl.setValue(""); // v-model.number → "" : le cas qui cassait sumPnl
    await wrapper.find("form").trigger("submit.prevent");
    expect(spy).not.toHaveBeenCalled();
  });

  it("soumet un trade valide (ticker en majuscules, pnl numérique)", async () => {
    const wrapper = mount(TradeForm);
    const session = useSessionStore();
    const spy = vi.spyOn(session, "addTrade").mockResolvedValue();
    await wrapper.find('input[placeholder="Ticker (ex. NQ)"]').setValue("nq");
    await wrapper.find('input[placeholder="Résultat (€/$)"]').setValue("25");
    await wrapper.find("form").trigger("submit.prevent");
    expect(spy).toHaveBeenCalledTimes(1);
    expect(spy.mock.calls[0][0].ticker).toBe("NQ");
    expect(spy.mock.calls[0][0].pnl).toBe(25);
  });

  it("calcule le pnl depuis taille/entrée/sortie (long) et persiste size/entry/exit", async () => {
    const wrapper = mount(TradeForm);
    const session = useSessionStore();
    const spy = vi.spyOn(session, "addTrade").mockResolvedValue();
    await wrapper.find('input[placeholder="Ticker (ex. NQ)"]').setValue("NQ");
    await wrapper.find('input[placeholder="Taille"]').setValue("2");
    await wrapper.find('input[placeholder="Entrée"]').setValue("100");
    await wrapper.find('input[placeholder="Sortie"]').setValue("110");
    await wrapper.find("form").trigger("submit.prevent");
    expect(spy).toHaveBeenCalledTimes(1);
    const arg = spy.mock.calls[0][0];
    expect(arg.pnl).toBe(20);
    expect(arg.size).toBe(2);
    expect(arg.entry).toBe(100);
    expect(arg.exit).toBe(110);
  });

  it("retient le ticker après un ajout", async () => {
    const wrapper = mount(TradeForm);
    const session = useSessionStore();
    vi.spyOn(session, "addTrade").mockResolvedValue();
    await wrapper.find('input[placeholder="Ticker (ex. NQ)"]').setValue("NQ");
    await wrapper.find('input[placeholder="Résultat (€/$)"]').setValue("15");
    await wrapper.find("form").trigger("submit.prevent");
    await wrapper.vm.$nextTick();
    expect(wrapper.find('input[placeholder="Ticker (ex. NQ)"]').element.value).toBe("NQ");
  });
});
