<%

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
        mean = timings.average;
        median = timings[timings.length / 2];
        std_dev = timings.map(\t { (t - mean) ** 2 }).average ** 0.5;
        print("\n");
        print("  trials:  #{trials}\n");
        print("  mean:    #{mean} sec\n");
        print("  std dev: #{std_dev} sec\n");
        print("  median:  #{median} sec\n");
    }
}
