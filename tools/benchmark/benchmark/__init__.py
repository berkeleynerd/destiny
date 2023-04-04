import logging
import sys
from benchmark.loader import load_benchmark


logging.basicConfig()


def run_benchmark(benchmark_name, iteration_count=None):
    try:
        benchmark = load_benchmark(
            benchmark_name,
            iteration_count=iteration_count
        )
    except RuntimeError as e:
        sys.stderr.write(e.message)
        sys.exit(1)

    benchmark.run()
