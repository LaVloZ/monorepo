import { describe, it, expect } from "vitest";
import { toRoman, fromRoman, MIN_ROMAN, MAX_ROMAN } from "./roman.js";

describe("toRoman", () => {
  it("converts the single-symbol values", () => {
    expect(toRoman(1)).toBe("I");
    expect(toRoman(5)).toBe("V");
    expect(toRoman(10)).toBe("X");
    expect(toRoman(50)).toBe("L");
    expect(toRoman(100)).toBe("C");
    expect(toRoman(500)).toBe("D");
    expect(toRoman(1000)).toBe("M");
  });

  it("uses subtractive notation for the four/nine cases", () => {
    expect(toRoman(4)).toBe("IV");
    expect(toRoman(9)).toBe("IX");
    expect(toRoman(40)).toBe("XL");
    expect(toRoman(90)).toBe("XC");
    expect(toRoman(400)).toBe("CD");
    expect(toRoman(900)).toBe("CM");
  });

  it("composes additive sequences", () => {
    expect(toRoman(2)).toBe("II");
    expect(toRoman(3)).toBe("III");
    expect(toRoman(8)).toBe("VIII");
    expect(toRoman(13)).toBe("XIII");
    expect(toRoman(2023)).toBe("MMXXIII");
  });

  it("handles the largest standard three-symbol numbers", () => {
    expect(toRoman(3888)).toBe("MMMDCCCLXXXVIII");
    expect(toRoman(3999)).toBe("MMMCMXCIX");
  });

  it("represents numbers above 3999 by repeating M for thousands", () => {
    expect(toRoman(4000)).toBe("MMMM");
    expect(toRoman(4999)).toBe("MMMMCMXCIX");
    expect(toRoman(9000)).toBe("MMMMMMMMM");
    expect(toRoman(10000)).toBe("MMMMMMMMMM");
  });

  it("accepts the boundary values", () => {
    expect(toRoman(MIN_ROMAN)).toBe("I");
    expect(toRoman(MAX_ROMAN)).toBe("MMMMMMMMMM");
  });

  it("rejects values below the minimum", () => {
    expect(() => toRoman(0)).toThrow(RangeError);
    expect(() => toRoman(-1)).toThrow(RangeError);
  });

  it("rejects values above the maximum", () => {
    expect(() => toRoman(10001)).toThrow(RangeError);
  });

  it("rejects non-integers", () => {
    expect(() => toRoman(1.5)).toThrow(RangeError);
    expect(() => toRoman(NaN)).toThrow(RangeError);
    expect(() => toRoman(Infinity)).toThrow(RangeError);
  });
});

describe("fromRoman", () => {
  it("parses single-symbol values", () => {
    expect(fromRoman("I")).toBe(1);
    expect(fromRoman("V")).toBe(5);
    expect(fromRoman("X")).toBe(10);
    expect(fromRoman("L")).toBe(50);
    expect(fromRoman("C")).toBe(100);
    expect(fromRoman("D")).toBe(500);
    expect(fromRoman("M")).toBe(1000);
  });

  it("parses subtractive notation", () => {
    expect(fromRoman("IV")).toBe(4);
    expect(fromRoman("IX")).toBe(9);
    expect(fromRoman("XL")).toBe(40);
    expect(fromRoman("XC")).toBe(90);
    expect(fromRoman("CD")).toBe(400);
    expect(fromRoman("CM")).toBe(900);
  });

  it("parses composed and large numbers", () => {
    expect(fromRoman("MMXXIII")).toBe(2023);
    expect(fromRoman("MMMCMXCIX")).toBe(3999);
    expect(fromRoman("MMMMMMMMMM")).toBe(10000);
  });

  it("is case-insensitive and trims surrounding whitespace", () => {
    expect(fromRoman("mmxxiii")).toBe(2023);
    expect(fromRoman("  XIV  ")).toBe(14);
  });

  it("rejects empty or whitespace-only input", () => {
    expect(() => fromRoman("")).toThrow(Error);
    expect(() => fromRoman("   ")).toThrow(Error);
  });

  it("rejects strings containing invalid characters", () => {
    expect(() => fromRoman("ABC")).toThrow(Error);
    expect(() => fromRoman("XII5")).toThrow(Error);
  });

  it("rejects malformed numerals that do not round-trip", () => {
    expect(() => fromRoman("IIII")).toThrow(Error);
    expect(() => fromRoman("VV")).toThrow(Error);
    expect(() => fromRoman("IL")).toThrow(Error);
    expect(() => fromRoman("IC")).toThrow(Error);
    expect(() => fromRoman("XatM")).toThrow(Error);
  });
});

describe("round-trip", () => {
  it("toRoman and fromRoman are inverses across the whole range", () => {
    for (let n = MIN_ROMAN; n <= MAX_ROMAN; n++) {
      expect(fromRoman(toRoman(n))).toBe(n);
    }
  });
});
