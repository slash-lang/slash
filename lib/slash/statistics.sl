<%

class Statistics {
    def self.variance(values) {
        mean = values.average;
        values.map(\t { (t - mean) ** 2 }).average;
    }

    def self.standard_deviation(values) {
        variance(values) ** 0.5;
    }
}
