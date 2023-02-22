import toto.Animal;
import toto.Animals;

class Song {

    public static final String THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A = "There was an old lady who swallowed a ";
    public static final String SHE_SWALLOWED_THE = "She swallowed the ";
    public static final String I_DON_T_KNOW_WHY_SHE_SWALLOWED_A = "I don't know why she swallowed a ";

    public static void main(String[] args) {

        System.out.println(Song.sing(
                new Animals(
                        new Animal("fly", ""),
                        new Animal("spider", "That wriggled and wiggled and tickled inside her."),
                        new Animal("bird", "How absurd to swallow a {name}."),
                        new Animal("cat", "Fancy that to swallow a {name}!"),
                        new Animal("dog", "What a hog, to swallow a {name}!"),
                        new Animal("cow", "I don't know how she swallowed a {name}!"),
                        new Animal("horse", ""))));
    }

    public static String sing(Animals animals) {
        return THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.fly() + ".\n" +
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
    }

}