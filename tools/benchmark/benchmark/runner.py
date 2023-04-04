from benchmark.output import IterationProgress, InitializationProgress, print_results, print_starting_benchmark
from benchmark.results import Duration, Results, NetworkStats


class Benchmark(object):
    def __init__(self, name, server, initializers, policies, iteration_count):
        self._name = name
        self._server = server
        self._initializers = initializers
        self._policies = policies
        self._policy_durations = []
        self._iteration_times = []
        self._tick_durations = []
        self._evolve_durations = []
        self._send_batch_durations = []
        self._iteration_count = iteration_count
        self._progress_bar = IterationProgress(iteration_count)
        self._initialization_seconds = 0.0

    def _run_iterations(self):
        iterations_done = 0
        while iterations_done < self._iteration_count:
            policy_duration = sum(policy.apply() for policy in self._policies)
            self._policy_durations.append(policy_duration)
            tick_duration, send_batch_duration, evolve_duration = self._server.tick()
            self._tick_durations.append(tick_duration)
            self._evolve_durations.append(evolve_duration)
            self._send_batch_durations.append(send_batch_duration)
            iteration_time = tick_duration + evolve_duration + policy_duration + send_batch_duration
            self._iteration_times.append(iteration_time)
            iterations_done += 1
            self._progress_bar.report_progress(iterations_done)

    def _collect_results(self):
        iteration_duration = Duration(self._iteration_times)
        policy_duration = Duration(self._policy_durations)
        tick_duration = Duration(self._tick_durations)
        evolve_duration = Duration(self._evolve_durations)
        send_batch_duration = Duration(self._send_batch_durations)
        singlecast_stats = NetworkStats(self._server.singlecast_bytes)
        narrowcast_stats = NetworkStats(self._server.narrowcast_bytes)
        return Results(
            iteration_duration,
            policy_duration,
            tick_duration,
            evolve_duration,
            send_batch_duration,
            singlecast_stats,
            narrowcast_stats
        )

    def _initialize(self):
        for initializer in self._initializers:
            progress = InitializationProgress(initializer)
            progress.start()
            initialization_seconds = initializer.initialize()
            progress.end(initialization_seconds)
            self._initialization_seconds += initialization_seconds

    def run(self):
        print_starting_benchmark(self._name, self._iteration_count)
        self._initialize()
        self._run_iterations()
        results = self._collect_results()
        print_results(results)
