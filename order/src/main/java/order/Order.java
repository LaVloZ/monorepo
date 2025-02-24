package order;

import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class Order {
    private final UUID orderId;
    private final List<OrderItem> items = new ArrayList<>();
    private OrderStatus status = OrderStatus.CREATED;

    public Order(UUID orderId) {
        this.orderId = orderId;
    }

    public void addItem(Product product, int quantity) {
        if (status != OrderStatus.CREATED) {
            throw new IllegalStateException("Can't add items to finalized orders");
        }
        items.add(new OrderItem(product, quantity));
    }

    public Money calculateTotal() {
        return items.stream()
                .map(OrderItem::getTotalPrice)
                .reduce(new Money(BigDecimal.ZERO, "USD"), Money::add);
    }

    public void completeOrder() {
        if (items.isEmpty()) {
            throw new IllegalStateException("Cannot complete order with no items");
        }
        this.status = OrderStatus.COMPLETED;
    }

    public OrderStatus getStatus() { return status; }
}
