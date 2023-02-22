import toto.Animals;

class Song {

    public static final String THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A = "There was an old lady who swallowed a ";
    public static final String SHE_SWALLOWED_THE = "She swallowed the ";
    public static final String I_DON_T_KNOW_WHY_SHE_SWALLOWED_A = "I don't know why she swallowed a ";

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
        String song = THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.fly() + ".\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + animals.fly() + " - perhaps she'll die!\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.spider() + ";\n" +
                animals.spider().rhyme() + "\n" +
                SHE_SWALLOWED_THE + animals.spider() + " to catch the " + animals.fly() + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + animals.fly() + " - perhaps she'll die!\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.bird() + ";\n" +
                animals.bird().rhyme() + "\n" +
                SHE_SWALLOWED_THE + animals.bird() + " to catch the " + animals.spider() + ",\n" +
                SHE_SWALLOWED_THE + animals.spider() + " to catch the " + animals.fly() + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + animals.fly() + " - perhaps she'll die!\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.cat() + ";\n" +
                animals.cat().rhyme() + "\n" +
                SHE_SWALLOWED_THE + animals.cat() + " to catch the " + animals.bird() + ",\n" +
                SHE_SWALLOWED_THE + animals.bird() + " to catch the " + animals.spider() + ",\n" +
                SHE_SWALLOWED_THE + animals.spider() + " to catch the " + animals.fly() + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + animals.fly() + " - perhaps she'll die!\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.dog() + ";\n" +
                animals.dog().rhyme() + "\n" +
                SHE_SWALLOWED_THE + animals.dog() + " to catch the "+ animals.cat() +",\n" +
                SHE_SWALLOWED_THE + animals.cat() + " to catch the " + animals.bird() + ",\n" +
                SHE_SWALLOWED_THE + animals.bird() + " to catch the " + animals.spider() + ",\n" +
                SHE_SWALLOWED_THE + animals.spider() + " to catch the " + animals.fly() + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + animals.fly() + " - perhaps she'll die!\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.cow() + ";\n" +
                animals.cow().rhyme() + "\n" +
                SHE_SWALLOWED_THE + animals.cow() + " to catch the " + animals.dog() + ",\n" +
                SHE_SWALLOWED_THE + animals.dog() + " to catch the " + animals.cat() +",\n" +
                SHE_SWALLOWED_THE + animals.cat() +" to catch the " + animals.bird() + ",\n" +
                SHE_SWALLOWED_THE + animals.bird() + " to catch the " + animals.spider() + ",\n" +
                SHE_SWALLOWED_THE + animals.spider() + " to catch the " + animals.fly() + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + animals.fly() + " - perhaps she'll die!\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.horse() + "...\n" +
                "...She's dead, of course!";
        return song;
    }

}