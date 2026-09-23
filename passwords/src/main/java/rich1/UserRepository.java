package rich1;

import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.jdbc.core.RowMapper;
import org.springframework.stereotype.Repository;

@Repository
public class UserRepository {
    private JdbcTemplate jdbcTemplate;

    public UserRepository(JdbcTemplate jdbcTemplate) {
        this.jdbcTemplate = jdbcTemplate;
    }

    public void createUser(User user) {
        jdbcTemplate.update("INSERT INTO User(USERNAME, PASSWORD) VALUES(?, ?)", user.getUsername(), user.getPassword());
    }

    public User getByUsername(String username) {
        return jdbcTemplate.queryForObject("SELECT * FROM User WHERE username like ?", userRowMapper, username);
    }

    private final RowMapper<User> userRowMapper = (rs, rowNum) -> new User(
            rs.getString("username"),
            rs.getString("password")
    );

    public void updateUser(User user) {
        jdbcTemplate.update("UPDATE User SET password = ? WHERE username like ?", user.getPassword(), user.getUsername());
    }
}
