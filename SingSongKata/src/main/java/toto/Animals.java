package toto;

import java.util.Arrays;
import java.util.List;
import java.util.Objects;

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
        return THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + fly() + ".\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + fly() + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + spider() + ";\n" +
                spider().rhyme() + "\n" +
                SHE_SWALLOWED_THE + spider() + TO_CATCH_THE + fly() + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + fly() + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + bird() + ";\n" +
                bird().rhyme() + "\n" +
                SHE_SWALLOWED_THE + bird() + TO_CATCH_THE + spider() + ",\n" +
                SHE_SWALLOWED_THE + spider() + TO_CATCH_THE + fly() + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + fly() + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + cat() + ";\n" +
                cat().rhyme() + "\n" +
                SHE_SWALLOWED_THE + cat() + TO_CATCH_THE + bird() + ",\n" +
                SHE_SWALLOWED_THE + bird() + TO_CATCH_THE + spider() + ",\n" +
                SHE_SWALLOWED_THE + spider() + TO_CATCH_THE + fly() + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + fly() + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + dog() + ";\n" +
                dog().rhyme() + "\n" +
                SHE_SWALLOWED_THE + dog() + TO_CATCH_THE + cat() + ",\n" +
                SHE_SWALLOWED_THE + cat() + TO_CATCH_THE + bird() + ",\n" +
                SHE_SWALLOWED_THE + bird() + TO_CATCH_THE + spider() + ",\n" +
                SHE_SWALLOWED_THE + spider() + TO_CATCH_THE + fly() + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + fly() + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + cow() + ";\n" +
                cow().rhyme() + "\n" +
                SHE_SWALLOWED_THE + cow() + TO_CATCH_THE + dog() + ",\n" +
                SHE_SWALLOWED_THE + dog() + TO_CATCH_THE + cat() + ",\n" +
                SHE_SWALLOWED_THE + cat() + TO_CATCH_THE + bird() + ",\n" +
                SHE_SWALLOWED_THE + bird() + TO_CATCH_THE + spider() + ",\n" +
                SHE_SWALLOWED_THE + spider() + TO_CATCH_THE + fly() + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + fly() + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + horse() + "...\n" +
                "...She's dead, of course!";
    }

    public Animal fly() {
        return animals.get(0);
    }

    public Animal spider() {
        return animals.get(1);
    }

    public Animal bird() {
        return animals.get(2);
    }

    public Animal cat() {
        return animals.get(3);
    }

    public Animal dog() {
        return animals.get(4);
    }

    public Animal cow() {
        return animals.get(5);
    }

    public Animal horse() {
        return animals.get(6);
    }
}