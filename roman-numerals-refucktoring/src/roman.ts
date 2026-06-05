/**
 * Simple, intuitive Roman numeral conversion for the range 1–10000.
 *
 * Standard Roman numerals only reach 3999 with distinct letters. To count up
 * to 10000 we keep the same intuitive rules and simply repeat `M` for each
 * thousand, so 4000 = "MMMM" and 10000 = "MMMMMMMMMM".
 */

export const MIN_ROMAN = 1;
export const MAX_ROMAN = 10000;

/** Value/symbol pairs, ordered from largest to smallest, including the
 *  subtractive forms (CM, CD, XC, XL, IX, IV). This single table drives both
 *  directions of the conversion. */
const NUMERALS: ReadonlyArray<readonly [number, string]> = [
  [1000, "M"],
  [900, "CM"],
  [500, "D"],
  [400, "CD"],
  [100, "C"],
  [90, "XC"],
  [50, "L"],
  [40, "XL"],
  [10, "X"],
  [9, "IX"],
  [5, "V"],
  [4, "IV"],
  [1, "I"],
];

/**
 * Convert an integer to its Roman numeral string.
 * @throws RangeError if `n` is not an integer in [1, 10000].
 */
export function toRoman(n: number): string {
  if (!Number.isInteger(n) || n < MIN_ROMAN || n > MAX_ROMAN) {
    throw new RangeError(
      `toRoman expects an integer in [${MIN_ROMAN}, ${MAX_ROMAN}], got ${n}`,
    );
  }

  let remaining = n;
  let result = "";
  for (const [value, symbol] of NUMERALS) {
    while (remaining >= value) {
      result += symbol;
      remaining -= value;
    }
  }
  return result;
}

const SINGLE_VALUES: Readonly<Record<string, number>> = {
  I: 1,
  V: 5,
  X: 10,
  L: 50,
  C: 100,
  D: 500,
  M: 1000,
};

/**
 * Parse a Roman numeral string back to an integer.
 *
 * Input is trimmed and case-insensitive. A value is accepted only if it is a
 * canonical numeral — i.e. it round-trips through {@link toRoman}. This rejects
 * malformed strings like "IIII", "VV", or "IL".
 *
 * @throws Error if the string is empty, contains invalid characters, or is not
 *   a canonical Roman numeral in [1, 10000].
 */
export function fromRoman(input: string): number {
  const normalized = input.trim().toUpperCase();
  if (normalized.length === 0) {
    throw new Error("fromRoman expects a non-empty Roman numeral");
  }

  let total = 0;
  for (let i = 0; i < normalized.length; i++) {
    const current = SINGLE_VALUES[normalized[i]];
    if (current === undefined) {
      throw new Error(`Invalid Roman numeral character: '${normalized[i]}'`);
    }
    const next = SINGLE_VALUES[normalized[i + 1]];
    // Subtractive notation: a smaller symbol before a larger one is subtracted.
    if (next !== undefined && current < next) {
      total -= current;
    } else {
      total += current;
    }
  }

  if (total < MIN_ROMAN || total > MAX_ROMAN || toRoman(total) !== normalized) {
    throw new Error(`Not a canonical Roman numeral: '${input}'`);
  }
  return total;
}
