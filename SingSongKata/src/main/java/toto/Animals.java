package toto;

public record Animals(Animal fly, Animal spider, Animal bird, Animal cat, Animal dog, Animal cow, Animal horse) {

    public static final String THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A = "There was an old lady who swallowed a ";
    public static final String SHE_SWALLOWED_THE = "She swallowed the ";
    public static final String I_DON_T_KNOW_WHY_SHE_SWALLOWED_A = "I don't know why she swallowed a ";
    public static final String TO_CATCH_THE = " to catch the ";
    public static final String PERHAPS_SHE_LL_DIE = " - perhaps she'll die!";

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
                SHE_SWALLOWED_THE + dog() + TO_CATCH_THE + cat() +",\n" +
                SHE_SWALLOWED_THE + cat() + TO_CATCH_THE + bird() + ",\n" +
                SHE_SWALLOWED_THE + bird() + TO_CATCH_THE + spider() + ",\n" +
                SHE_SWALLOWED_THE + spider() + TO_CATCH_THE + fly() + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + fly() + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + cow() + ";\n" +
                cow().rhyme() + "\n" +
                SHE_SWALLOWED_THE + cow() + TO_CATCH_THE + dog() + ",\n" +
                SHE_SWALLOWED_THE + dog() + TO_CATCH_THE + cat() +",\n" +
                SHE_SWALLOWED_THE + cat() + TO_CATCH_THE + bird() + ",\n" +
                SHE_SWALLOWED_THE + bird() + TO_CATCH_THE + spider() + ",\n" +
                SHE_SWALLOWED_THE + spider() + TO_CATCH_THE + fly() + ";\n" +
                I_DON_T_KNOW_WHY_SHE_SWALLOWED_A + fly() + PERHAPS_SHE_LL_DIE + "\n" +
                "\n" +
                THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A + horse() + "...\n" +
                "...She's dead, of course!";
    }
}