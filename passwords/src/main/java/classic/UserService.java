package classic;

import org.springframework.stereotype.Service;

@Service
public class UserService {
    public UserService(UserRepository userRepository) {
        this.userRepository = userRepository;
    }

    private UserRepository userRepository;

    public void createUser(User user) {
        userRepository.createUser(user);
    }

    public void updateUser(String username, String password) {
        User user = userRepository.getByUsername(username);
        user.setPassword(password);
        userRepository.updateUser(user);
    }

    public User getByUsername(String username) {
        return userRepository.getByUsername(username);
    }
}
