<%

use Statistics;

class Benchmark {
    def self.measure(fun) {
        start = Time.clock;
        fun.call;
        finish = Time.clock;
        (finish - start).to_f / Time::CLOCKS_PER_SEC;
    }

    def self.[](name, fun, trials = 5) {
        print("#{name}: ");

        timings = (1..trials).map(\i {
            sec = measure(fun);
            print(".");
            sec;
        }).sort;

        mean = Statistics.average(timings);
        median = Statistics.median(timings);
        std_dev = Statistics.standard_deviation(timings);

        print("\n");
        print("  trials:  #{trials}\n");
        print("  mean:    #{mean} sec\n");
        print("  std dev: #{std_dev} sec\n");
        print("  median:  #{median} sec\n");
    }
}
