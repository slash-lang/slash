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

    def test_median {
        assert_equal(nil, Statistics.median([]));
        assert_equal(123, Statistics.median([123]));
        assert_equal(1, Statistics.median([-2, 100, 1]));
    }

    def test_average {
        assert_equal(nil, Statistics.average([]));
        assert_equal(0.0, Statistics.average([0]));
        assert_equal(1.5, Statistics.average([1, 2]));
    }
}
