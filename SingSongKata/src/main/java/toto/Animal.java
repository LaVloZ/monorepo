package toto;

public class Animal {
    private final String name;

    public Animal(String cat) {
        this.name = cat;
    }

    public String rhyme() {
        if(name == "cat")
            return "Fancy that to swallow a " + name + "!";
        return  "How absurd to swallow a " + name + ".";
    }

    @Override
    public String toString() {
        return name;
    }
}
