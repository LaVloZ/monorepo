package toto;

public class Animal {
    private final String name;

    public Animal(String cat) {
        this.name = cat;
    }

    public static String rhymeCat(Animals animals) {
        return "Fancy that to swallow a " + animals.cat() + "!";
    }

    @Override
    public String toString() {
        return name;
    }
}
