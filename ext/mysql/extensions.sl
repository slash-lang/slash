<%

class MySQL {
    def databases {
        query("SHOW DATABASES").map(\row . row["Database"]);
    }

    def select_one(*args) {
        query(*args).first;
    }

    def select_value(*args) {
        if row = select_one(*args) {
            row[row.keys.first];
        }
    }
}
