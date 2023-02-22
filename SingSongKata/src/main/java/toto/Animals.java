package toto;

import java.util.Objects;

public final class Animals {

    public static final String THERE_WAS_AN_OLD_LADY_WHO_SWALLOWED_A = "There was an old lady who swallowed a ";
    public static final String SHE_SWALLOWED_THE = "She swallowed the ";
    public static final String I_DON_T_KNOW_WHY_SHE_SWALLOWED_A = "I don't know why she swallowed a ";
    public static final String TO_CATCH_THE = " to catch the ";
    public static final String PERHAPS_SHE_LL_DIE = " - perhaps she'll die!";
    private final Animal fly;
    private final Animal spider;
    private final Animal bird;
    private final Animal cat;
    private final Animal dog;
    private final Animal cow;
    private final Animal horse;

    public Animals(Animal fly, Animal spider, Animal bird, Animal cat, Animal dog, Animal cow, Animal horse) {
        this.fly = fly;
        this.spider = spider;
        this.bird = bird;
        this.cat = cat;
        this.dog = dog;
        this.cow = cow;
        this.horse = horse;
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
        return fly;
    }

    public Animal spider() {
        return spider;
    }

    public Animal bird() {
        return bird;
    }

    public Animal cat() {
        return cat;
    }

    public Animal dog() {
        return dog;
    }

    public Animal cow() {
        return cow;
    }

    public Animal horse() {
        return horse;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == this) return true;
        if (obj == null || obj.getClass() != this.getClass()) return false;
        var that = (Animals) obj;
        return Objects.equals(this.fly, that.fly) &&
                Objects.equals(this.spider, that.spider) &&
                Objects.equals(this.bird, that.bird) &&
                Objects.equals(this.cat, that.cat) &&
                Objects.equals(this.dog, that.dog) &&
                Objects.equals(this.cow, that.cow) &&
                Objects.equals(this.horse, that.horse);
    }

    @Override
    public int hashCode() {
        return Objects.hash(fly, spider, bird, cat, dog, cow, horse);
    }

    @Override
    public String toString() {
        return "Animals[" +
                "fly=" + fly + ", " +
                "spider=" + spider + ", " +
                "bird=" + bird + ", " +
                "cat=" + cat + ", " +
                "dog=" + dog + ", " +
                "cow=" + cow + ", " +
                "horse=" + horse + ']';
    }

}