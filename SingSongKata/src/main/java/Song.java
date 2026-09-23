import toto.Animal;
import toto.Animals;

class Song {

    public static void main(String[] args) {

        System.out.println(new Animals(
                new Animal("fly", ""),
                new Animal("spider", "That wriggled and wiggled and tickled inside her."),
                new Animal("bird", "How absurd to swallow a {name}."),
                new Animal("cat", "Fancy that to swallow a {name}!"),
                new Animal("dog", "What a hog, to swallow a {name}!"),
                new Animal("cow", "I don't know how she swallowed a {name}!"),
                new Animal("horse", "")).sing(
        ));
    }

}