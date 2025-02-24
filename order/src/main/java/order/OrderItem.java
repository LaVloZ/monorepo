package order;

import java.math.BigDecimal;

public class OrderItem {
    private final Product product;
    private final int quantity;

    public OrderItem(Product product, int quantity) {
        if (quantity <= 0) throw new IllegalArgumentException("Quantity must be positive");
        this.product = product;
        this.quantity = quantity;
    }

    public Money getTotalPrice() {
        return product.getPrice().add(new Money(product.getPrice().getAmount().multiply(BigDecimal.valueOf(quantity - 1)), product.getPrice().getCurrency()));
    }
}
