import org.junit.jupiter.api.Test;
import toto.Animal;
import toto.Animals;

import static org.junit.jupiter.api.Assertions.*;
class SongTest {
  @Test
  public void toto(){
      assertEquals("There was an old lady who swallowed a fly.\n" +
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
                      "...She's dead, of course!", Song.sing(
                              new Animals(
                                      new Animal("fly", ""),
                                      new Animal("spider", "That wriggled and wiggled and tickled inside her."),
                                      new Animal("bird", "How absurd to swallow a {name}."),
                                      new Animal("cat", "Fancy that to swallow a {name}!"),
                                      new Animal("dog", "What a hog, to swallow a {name}!"),
                                      new Animal("cow", "I don't know how she swallowed a {name}!"),
                                      new Animal("horse", ""))));
  }
}