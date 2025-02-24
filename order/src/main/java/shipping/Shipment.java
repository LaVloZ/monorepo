package shipping;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class Shipment {
    private final UUID shipmentId;
    private final List<Package> packages = new ArrayList<>();
    private ShipmentStatus status = ShipmentStatus.CREATED;
    private final Weight weightLimit = new Weight(50.0); // Max 50kg per shipment

    public Shipment(UUID shipmentId) {
        this.shipmentId = shipmentId;
    }

    public void addPackage(Package pkg) {
        if (status != ShipmentStatus.CREATED) {
            throw new IllegalStateException("Cannot add packages after shipment is in transit");
        }

        Weight totalWeight = calculateTotalWeight().add(pkg.getWeight());
        if (totalWeight.getValue() > weightLimit.getValue()) {
            throw new IllegalArgumentException("Shipment weight limit exceeded");
        }

        packages.add(pkg);
    }

    public void startTransit() {
        if (packages.isEmpty()) {
            throw new IllegalStateException("Cannot start transit without packages");
        }
        this.status = ShipmentStatus.IN_TRANSIT;
    }

    public void markPackageAsDelivered(UUID packageId) {
        packages.stream()
                .filter(pkg -> pkg.getPackageId().equals(packageId))
                .findFirst()
                .orElseThrow(() -> new IllegalArgumentException("Package not found"))
                .markAsDelivered();

        if (allPackagesDelivered()) {
            this.status = ShipmentStatus.DELIVERED;
        }
    }

    private boolean allPackagesDelivered() {
        return packages.stream().allMatch(Package::isDelivered);
    }

    public Weight calculateTotalWeight() {
        return packages.stream()
                .map(Package::getWeight)
                .reduce(new Weight(0), Weight::add);
    }

    public ShipmentStatus getStatus() {
        return status;
    }
}
