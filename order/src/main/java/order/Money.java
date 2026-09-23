package order;

import java.math.BigDecimal;

// Value Object
public class Money {
    private final BigDecimal amount;
    private final String currency;

    public Money(BigDecimal amount, String currency) {
        if (amount.compareTo(BigDecimal.ZERO) < 0) {
            throw new IllegalArgumentException("Amount cannot be negative");
        }
        this.amount = amount;
        this.currency = currency;
    }

    public Money add(Money other) {
        if (!this.currency.equals(other.currency)) {
            throw new IllegalArgumentException("Currencies must match");
        }
        return new Money(this.amount.add(other.amount), currency);
    }

    public BigDecimal getAmount() { return amount; }
    public String getCurrency() { return currency; }
}
