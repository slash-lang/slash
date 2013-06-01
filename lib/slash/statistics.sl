<%

class Statistics {
    def self.variance(values) {
        mean = values.average;
        values.map(\t { (t - mean) ** 2 }).average;
    }

    def self.standard_deviation(values) {
        if var = variance(values) {
            var ** 0.5;
        }
    }
}
