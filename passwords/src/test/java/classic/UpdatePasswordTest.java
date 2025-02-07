package classic;

import config.SpringConfig;
import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;

@SpringBootTest(classes = SpringConfig.class)
public class UpdatePasswordTest {

    @Autowired
    UserService userService;

    @Test
    public void updatePasswordTest() {
        User spongebob = new User("SpongeBob", "Old Password");
        userService.createUser(spongebob);

        userService.updateUser("SpongeBob", "New Password");

        Assertions.assertThat(userService.getByUsername("SpongeBob"))
                .extracting(User::getPassword)
                .isEqualTo("New Passowrd");
    }
}
