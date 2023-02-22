package toto;

import java.util.Arrays;
import java.util.List;

public final class Animals {

    public static final String THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A = "There was an old lady who swallowed a ";
    public static final String SHE_SWALLOWED_THE = "She swallowed the ";
    public static final String I_DON_T_KNOW_WHY_SHE_SWALLOWED_A = "I don't know why she swallowed a ";
    public static final String TO_CATCH_THE = " to catch the ";
    public static final String PERHAPS_SHE_LL_DIE = " - perhaps she'll die!";
    private final List<Animal> animals;

    public Animals(Animal... animals) {
        this.animals = Arrays.asList(animals);
    }

    public String sing() {
        String song = THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.get(0) + ".\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + animals.get(0) + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +

                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.get(1) + ";\n" +
                animals.get(1).rhyme() + "\n" +
                SHE_SWALLOWED_THE + animals.get(1) + TO_CATCH_THE + animals.get(0) + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + animals.get(0) + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +

                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.get(2) + ";\n" +
                animals.get(2).rhyme() + "\n" +
                SHE_SWALLOWED_THE + animals.get(2) + TO_CATCH_THE + animals.get(1) + ",\n" +
                SHE_SWALLOWED_THE + animals.get(1) + TO_CATCH_THE + animals.get(0) + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + animals.get(0) + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +

                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.get(3) + ";\n" +
                animals.get(3).rhyme() + "\n" +
                SHE_SWALLOWED_THE + animals.get(3) + TO_CATCH_THE + animals.get(2) + ",\n" +
                SHE_SWALLOWED_THE + animals.get(2) + TO_CATCH_THE + animals.get(1) + ",\n" +
                SHE_SWALLOWED_THE + animals.get(1) + TO_CATCH_THE + animals.get(0) + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + animals.get(0) + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +

                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.get(4) + ";\n" +
                animals.get(4).rhyme() + "\n" +
                SHE_SWALLOWED_THE + animals.get(4) + TO_CATCH_THE + animals.get(3) + ",\n" +
                SHE_SWALLOWED_THE + animals.get(3) + TO_CATCH_THE + animals.get(2) + ",\n" +
                SHE_SWALLOWED_THE + animals.get(2) + TO_CATCH_THE + animals.get(1) + ",\n" +
                SHE_SWALLOWED_THE + animals.get(1) + TO_CATCH_THE + animals.get(0) + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + animals.get(0) + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +

                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.get(5) + ";\n" +
                animals.get(5).rhyme() + "\n";


                for (int i = 5; i > 0; i--) {
                    song += SHE_SWALLOWED_THE + animals.get(i) + TO_CATCH_THE + animals.get(i - 1) + (i == 1 ? ';' : ',') + "\n";
                }
                song += I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + animals.get(0) + PERHAPS_SHE_LL_DIE + "\n";
                song += "\n" +

                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + animals.get(6) + "...\n" +
                "...She's dead, of course!";

        return song;
    }

}