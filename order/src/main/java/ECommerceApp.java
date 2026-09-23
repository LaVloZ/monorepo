import order.Money;
import order.Order;
import order.Product;

import java.math.BigDecimal;
import java.util.UUID;

public class ECommerceApp {
    public static void main(String[] args) {
        Product laptop = new Product(UUID.randomUUID(), "Laptop", new Money(new BigDecimal("999.99"), "USD"));
        Product mouse = new Product(UUID.randomUUID(), "Mouse", new Money(new BigDecimal("49.99"), "USD"));

        Order order = new Order(UUID.randomUUID());
        order.addItem(laptop, 1);
        order.addItem(mouse, 2);

        System.out.println("order.Order Total: $" + order.calculateTotal().getAmount());

        order.completeOrder();
        System.out.println("order.Order Status: " + order.getStatus());
    }
}