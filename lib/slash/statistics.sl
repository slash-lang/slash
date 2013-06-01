<%

class Statistics {
    def self.average(values) {
        return nil if values.empty;
        values.sum.to_f / values.length;
    }

    def self.median(values) {
        values = values.sort;
        values[values.length / 2];
    }

    def self.variance(values) {
        mean = average(values);
        average(values.map(\t { (t - mean) ** 2 }));
    }

    def self.standard_deviation(values) {
        if var = variance(values) {
            var ** 0.5;
        }
    }
}
