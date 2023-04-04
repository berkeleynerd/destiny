from benchmark.util import mean, stddev


class Duration(object):
    def __init__(self, duration_list):
        self.avg = mean(duration_list)
        self.stddev = stddev(duration_list)
        self.min = min(duration_list) if duration_list else None
        self.max = max(duration_list) if duration_list else None


class NetworkStats(object):
    def __init__(self, bytes_list):
        self.count = len(bytes_list)
        self.bytes_list = bytes_list
        self.avg = mean(bytes_list)
        self.stddev = stddev(bytes_list)
        self.min = min(bytes_list) if bytes_list else None
        self.max = max(bytes_list) if bytes_list else None


class Results(object):
    def __init__(
            self,
            iteration_duration,
            policy_duration,
            tick_duration,
            evolve_duration,
            send_batch_duration,
            singlecast_stats,
            narrowcast_stats):
        self.iteration_duration = iteration_duration
        self.policy_duration = policy_duration
        self.tick_duration = tick_duration
        self.evolve_duration = evolve_duration
        self.send_batch_duration = send_batch_duration
        self.singlecast_stats = singlecast_stats
        self.narrowcast_stats = narrowcast_stats
