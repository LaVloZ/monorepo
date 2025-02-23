package rich1;

import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.transaction.annotation.Transactional;
import rich1.config.SpringConfig;

@SpringBootTest(classes = SpringConfig.class)
@Transactional
public class UpdatePasswordTest {

    @Autowired
    UserService userService;

    @Test
    public void update_password_hashed() {
        User spongebob = new User("SpongeBob", "Old Password");
        userService.createUser(spongebob);

        userService.updateUser("SpongeBob", "New Password");

        Assertions.assertThat(userService.getByUsername("SpongeBob"))
                .extracting(User::getPassword)
                .isEqualTo("ae3bb2a1ac61750150b606298091d38a");
    }
}
