package shipping;

import java.util.UUID;

public class Package {
    private final UUID packageId;
    private final String description;
    private final Weight weight;
    private boolean delivered;

    public Package(UUID packageId, String description, Weight weight) {
        this.packageId = packageId;
        this.description = description;
        this.weight = weight;
        this.delivered = false;
    }

    public void markAsDelivered() {
        this.delivered = true;
    }

    public boolean isDelivered() {
        return delivered;
    }

    public Weight getWeight() {
        return weight;
    }

    public UUID getPackageId() {
        return packageId;
    }
}
