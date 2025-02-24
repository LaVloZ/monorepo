package order;

import java.util.UUID;

// Entity
public class Product {
    private final UUID id;
    private final String name;
    private final Money price;

    public Product(UUID id, String name, Money price) {
        this.id = id;
        this.name = name;
        this.price = price;
    }

    public Money getPrice() { return price; }
    public String getName() { return name; }
}
