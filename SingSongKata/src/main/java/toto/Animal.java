package toto;

public class Animal {
    private final String name;

    public Animal(String cat) {
        this.name = cat;
    }

    public String rhymeCat() {
        return "Fancy that to swallow a " + name + "!";
    }

    @Override
    public String toString() {
        return name;
    }
}
