<%

use Statistics;

class StatisticsTest extends Test {
    def test_variance {
        assert_equal(nil, Statistics.variance([]));
        assert_equal(2, Statistics.variance([1,2,3,4,5]));
    }

    def test_standard_deviation {
        assert_equal(nil, Statistics.standard_deviation([]));
        assert_equal(2 ** 0.5, Statistics.standard_deviation([1,2,3,4,5]));
    }
}
