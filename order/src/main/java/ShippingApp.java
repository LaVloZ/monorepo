import shipping.Package;
import shipping.Shipment;
import shipping.Weight;

import java.util.UUID;

public class ShippingApp {
    public static void main(String[] args) {
        Shipment shipment = new Shipment(UUID.randomUUID());

        Package box1 = new Package(UUID.randomUUID(), "Electronics", new Weight(10.5));
        Package box2 = new Package(UUID.randomUUID(), "Books", new Weight(12.0));
        Package box3 = new Package(UUID.randomUUID(), "Clothes", new Weight(8.0));

        shipment.addPackage(box1);
        shipment.addPackage(box2);
        shipment.addPackage(box3);

        System.out.println("Total Weight: " + shipment.calculateTotalWeight().getValue() + " kg");
        shipment.startTransit();
        System.out.println("Shipment Status: " + shipment.getStatus());

        // Mark packages as delivered
        shipment.markPackageAsDelivered(box1.getPackageId());
        shipment.markPackageAsDelivered(box2.getPackageId());
        shipment.markPackageAsDelivered(box3.getPackageId());

        System.out.println("Final Shipment Status: " + shipment.getStatus());
    }
}
