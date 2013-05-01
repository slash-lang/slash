<%

class MySQL {
    def databases {
        query("SHOW DATABASES").map(\row . row["Database"]);
    }
}

