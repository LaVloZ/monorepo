import toto.Animals;

class Song {

    public static void main(String[] args) {

        String song = "There was an old lady who swallowed a fly.\n" +
                "I don't know why she swallowed a fly - perhaps she'll die!\n" +
                "\n" +
                "There was an old lady who swallowed a spider;\n" +
                "That wriggled and wiggled and tickled inside her.\n" +
                "She swallowed the spider to catch the fly;\n" +
                "I don't know why she swallowed a fly - perhaps she'll die!\n" +
                "\n" +
                "There was an old lady who swallowed a bird;\n" +
                "How absurd to swallow a bird.\n" +
                "She swallowed the bird to catch the spider,\n" +
                "She swallowed the spider to catch the fly;\n" +
                "I don't know why she swallowed a fly - perhaps she'll die!\n" +
                "\n" +
                "There was an old lady who swallowed a cat;\n" +
                "Fancy that to swallow a cat!\n" +
                "She swallowed the cat to catch the bird,\n" +
                "She swallowed the bird to catch the spider,\n" +
                "She swallowed the spider to catch the fly;\n" +
                "I don't know why she swallowed a fly - perhaps she'll die!\n" +
                "\n" +
                "There was an old lady who swallowed a dog;\n" +
                "What a hog, to swallow a dog!\n" +
                "She swallowed the dog to catch the cat,\n" +
                "She swallowed the cat to catch the bird,\n" +
                "She swallowed the bird to catch the spider,\n" +
                "She swallowed the spider to catch the fly;\n" +
                "I don't know why she swallowed a fly - perhaps she'll die!\n" +
                "\n" +
                "There was an old lady who swallowed a cow;\n" +
                "I don't know how she swallowed a cow!\n" +
                "She swallowed the cow to catch the dog,\n" +
                "She swallowed the dog to catch the cat,\n" +
                "She swallowed the cat to catch the bird,\n" +
                "She swallowed the bird to catch the spider,\n" +
                "She swallowed the spider to catch the fly;\n" +
                "I don't know why she swallowed a fly - perhaps she'll die!\n" +
                "\n" +
                "There was an old lady who swallowed a horse...\n" +
                "...She's dead, of course!";

        System.out.println(song);
    }

    public static String sing(Animals animals) {
        String song = "There was an old lady who swallowed a " + animals.fly() + ".\n" +
                "I don't know why she swallowed a " + animals.fly() + " - perhaps she'll die!\n" +
                "\n" +
                "There was an old lady who swallowed a " + animals.spider() + ";\n" +
                "That wriggled and wiggled and tickled inside her.\n" +
                "She swallowed the " + animals.spider() + " to catch the " + animals.fly() + ";\n" +
                "I don't know why she swallowed a " + animals.fly() + " - perhaps she'll die!\n" +
                "\n" +
                "There was an old lady who swallowed a " + animals.bird() + ";\n" +
                "How absurd to swallow a " + animals.bird() + ".\n" +
                "She swallowed the " + animals.bird() + " to catch the " + animals.spider() + ",\n" +
                "She swallowed the " + animals.spider() + " to catch the " + animals.fly() + ";\n" +
                "I don't know why she swallowed a " + animals.fly() + " - perhaps she'll die!\n" +
                "\n" +
                "There was an old lady who swallowed a " + animals.cat() + ";\n" +
                "Fancy that to swallow a " + animals.cat() + "!\n" +
                "She swallowed the "+ animals.cat() + " to catch the " + animals.bird() + ",\n" +
                "She swallowed the " + animals.bird() + " to catch the " + animals.spider() + ",\n" +
                "She swallowed the " + animals.spider() + " to catch the " + animals.fly() + ";\n" +
                "I don't know why she swallowed a " + animals.fly() + " - perhaps she'll die!\n" +
                "\n" +
                "There was an old lady who swallowed a " + animals.dog() + ";\n" +
                "What a hog, to swallow a " + animals.dog() + "!\n" +
                "She swallowed the " + animals.dog() + " to catch the "+ animals.cat() +",\n" +
                "She swallowed the " + animals.cat() + " to catch the " + animals.bird() + ",\n" +
                "She swallowed the " + animals.bird() + " to catch the " + animals.spider() + ",\n" +
                "She swallowed the " + animals.spider() + " to catch the " + animals.fly() + ";\n" +
                "I don't know why she swallowed a " + animals.fly() + " - perhaps she'll die!\n" +
                "\n" +
                "There was an old lady who swallowed a " + animals.cow() + ";\n" +
                "I don't know how she swallowed a " + animals.cow() + "!\n" +
                "She swallowed the " + animals.cow() + " to catch the " + animals.dog() + ",\n" +
                "She swallowed the " + animals.dog() + " to catch the " + animals.cat() +",\n" +
                "She swallowed the " + animals.cat() +" to catch the " + animals.bird() + ",\n" +
                "She swallowed the " + animals.bird() + " to catch the " + animals.spider() + ",\n" +
                "She swallowed the " + animals.spider() + " to catch the " + animals.fly() + ";\n" +
                "I don't know why she swallowed a " + animals.fly() + " - perhaps she'll die!\n" +
                "\n" +
                "There was an old lady who swallowed a " + animals.horse() + "...\n" +
                "...She's dead, of course!";
        return song;
    }
}