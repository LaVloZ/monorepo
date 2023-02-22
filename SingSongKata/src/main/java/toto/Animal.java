package toto;

public class Animal {
    private final String name;
    private String rhyme;

    public Animal(String cat, String rhyme) {
        this.name = cat;
        this.rhyme = rhyme;
    }

    public Animal(String cat) {
        this.name = cat;
        this.rhyme = "";
    }

    public String rhyme() {
        if(name == "cat")
            return "Fancy that to swallow a " + name + "!";
        if (name == "spider")
            return "That wriggled and wiggled and tickled inside her.";
        return rhyme.replaceAll("\\{name\\}", name);
    }

    @Override
    public String toString() {
        return name;
    }
}
