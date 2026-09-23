package shipping;

public class Weight {
    private final double value; // in kilograms

    public Weight(double value) {
        if (value <= 0) {
            throw new IllegalArgumentException("Weight must be positive");
        }
        this.value = value;
    }

    public double getValue() {
        return value;
    }

    public Weight add(Weight other) {
        return new Weight(this.value + other.value);
    }
}
