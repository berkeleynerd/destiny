from benchmark.runner import Benchmark
import benchmark.initializer
import benchmark.policy
from benchmark.server import Server
import yaml
import os


DEFAULT_ITERATION_COUNT = 1000


def load_benchmark(benchmark_file_path, iteration_count=None):
    if not os.path.exists(benchmark_file_path):
        raise RuntimeError("Could not find file '{}'".format(benchmark_file_path))
    benchmark_file_name = os.path.split(benchmark_file_path)[-1]
    benchmark_name = benchmark_file_name.split(".")[0]

    server = Server()
    initializers = []
    policies = []

    with open(benchmark_file_path, "r") as f:
        data = yaml.load(f)

    for initializer_data in data.get("initializers", []):
        initializer_name = initializer_data[0]
        try:
            kwargs = initializer_data[1]
        except IndexError:
            kwargs = {}
        if len(initializer_data) > 2:
            raise ValueError("Malformed initializer data. Kwargs should be a single list, not multiple additional list entries.")
        initializer_class = getattr(benchmark.initializer, initializer_name, None)
        if not issubclass(initializer_class, benchmark.initializer.Initializer):
            raise RuntimeError("Invalid initializer name '{}'".format(initializer_name))
        initializer = initializer_class(server, **kwargs)
        initializers.append(initializer)

    for policy_data in data.get("policies", []):
        policy_name = policy_data[0]
        policy_class = getattr(benchmark.policy, policy_name, None)
        if not issubclass(policy_class, benchmark.policy.Policy):
            raise RuntimeError("Invalid policy name '{}'".format(policy_name))
        try:
            kwargs = policy_data[1]
        except IndexError:
            kwargs = {}
        policy = policy_class(server, **kwargs)
        policies.append(policy)

    if iteration_count is None:
        iteration_count = data.get("iteration_count", DEFAULT_ITERATION_COUNT)
    return Benchmark(benchmark_name, server, initializers, policies, iteration_count)
