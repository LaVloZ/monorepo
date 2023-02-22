import toto.Animal;
import toto.Animals;

class Song {

    public static final String THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A = "There was an old lady who swallowed a ";
    public static final String SHE_SWALLOWED_THE = "She swallowed the ";
    public static final String I_DON_T_KNOW_WHY_SHE_SWALLOWED_A = "I don't know why she swallowed a ";
    public static final String TO_CATCH_THE = " to catch the ";
    public static final String PERHAPS_SHE_LL_DIE = " - perhaps she'll die!";

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
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + animals.fly() + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.spider() + ";\n" +
                animals.spider().rhyme() + "\n" +
                SHE_SWALLOWED_THE + animals.spider() + TO_CATCH_THE + animals.fly() + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + animals.fly() + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.bird() + ";\n" +
                animals.bird().rhyme() + "\n" +
                SHE_SWALLOWED_THE + animals.bird() + TO_CATCH_THE + animals.spider() + ",\n" +
                SHE_SWALLOWED_THE + animals.spider() + TO_CATCH_THE + animals.fly() + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + animals.fly() + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.cat() + ";\n" +
                animals.cat().rhyme() + "\n" +
                SHE_SWALLOWED_THE + animals.cat() + TO_CATCH_THE + animals.bird() + ",\n" +
                SHE_SWALLOWED_THE + animals.bird() + TO_CATCH_THE + animals.spider() + ",\n" +
                SHE_SWALLOWED_THE + animals.spider() + TO_CATCH_THE + animals.fly() + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + animals.fly() + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.dog() + ";\n" +
                animals.dog().rhyme() + "\n" +
                SHE_SWALLOWED_THE + animals.dog() + TO_CATCH_THE + animals.cat() +",\n" +
                SHE_SWALLOWED_THE + animals.cat() + TO_CATCH_THE + animals.bird() + ",\n" +
                SHE_SWALLOWED_THE + animals.bird() + TO_CATCH_THE + animals.spider() + ",\n" +
                SHE_SWALLOWED_THE + animals.spider() + TO_CATCH_THE + animals.fly() + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + animals.fly() + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.cow() + ";\n" +
                animals.cow().rhyme() + "\n" +
                SHE_SWALLOWED_THE + animals.cow() + TO_CATCH_THE + animals.dog() + ",\n" +
                SHE_SWALLOWED_THE + animals.dog() + TO_CATCH_THE + animals.cat() +",\n" +
                SHE_SWALLOWED_THE + animals.cat() + TO_CATCH_THE + animals.bird() + ",\n" +
                SHE_SWALLOWED_THE + animals.bird() + TO_CATCH_THE + animals.spider() + ",\n" +
                SHE_SWALLOWED_THE + animals.spider() + TO_CATCH_THE + animals.fly() + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + animals.fly() + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.horse() + "...\n" +
                "...She's dead, of course!";
    }

}