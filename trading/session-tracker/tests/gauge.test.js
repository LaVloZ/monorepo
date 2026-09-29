import { describe, it, expect } from "vitest";
import { mount } from "@vue/test-utils";
import Gauge from "../src/components/Gauge.vue";
import StopBanner from "../src/components/StopBanner.vue";

describe("Gauge", () => {
  it("remplit à la bonne fraction (clampée)", () => {
    const w = mount(Gauge, { props: { label: "Loss", value: 40, max: 100, variant: "loss" } });
    expect(w.find(".fill").attributes("style")).toContain("40%");
  });
  it("clampe au-dessus de 100%", () => {
    const w = mount(Gauge, { props: { label: "Loss", value: 150, max: 100, variant: "loss" } });
    expect(w.find(".fill").attributes("style")).toContain("100%");
  });
  it("warn force la couleur ambre par-dessus le variant", () => {
    const w = mount(Gauge, { props: { label: "Loss", value: 40, max: 100, variant: "gain", warn: true } });
    expect(w.find(".fill").classes()).toContain("amber");
  });
  it("max <= 0 donne 0%", () => {
    const w = mount(Gauge, { props: { label: "Loss", value: 40, max: 0, variant: "loss" } });
    expect(w.find(".fill").attributes("style")).toContain("0%");
  });
});

describe("StopBanner", () => {
  it("n'affiche rien sans raison", () => {
    const w = mount(StopBanner, { props: { reasons: [] } });
    expect(w.text()).toBe("");
  });
  it("affiche STOP TRADING avec des raisons", () => {
    const w = mount(StopBanner, { props: { reasons: ["lossLimit"] } });
    expect(w.text()).toContain("STOP TRADING");
  });
});
