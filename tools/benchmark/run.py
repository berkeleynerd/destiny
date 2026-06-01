# Copyright © 2023 CCP ehf.

import argparse
from benchmark import run_benchmark


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "benchmark_file_path",
        help="The path to the file containing the benchmark."
    )
    parser.add_argument(
        "--iterations",
        type=int,
        default=None,
        help="Specify the number of ticks to run for."
    )
    args = parser.parse_args()
    run_benchmark(args.benchmark_file_path, iteration_count=args.iterations)


if __name__ == '__main__':
    main()
