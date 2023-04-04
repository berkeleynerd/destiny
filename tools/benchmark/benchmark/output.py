import destiny
import geo2

KB = 1000
MB = KB * 1000
GB = MB * 1000
TB = GB * 1000


MODE_TO_STRING = {
    destiny.DSTBALL_BOID: 'DSTBALL_BOID',
    destiny.DSTBALL_FIELD: 'DSTBALL_FIELD',
    destiny.DSTBALL_FOLLOW: 'DSTBALL_FOLLOW',
    destiny.DSTBALL_FORMATION: 'DSTBALL_FORMATION',
    destiny.DSTBALL_GOTO: 'DSTBALL_GOTO',
    destiny.DSTBALL_MINIBALL: 'DSTBALL_MINIBALL',
    destiny.DSTBALL_MISSILE: 'DSTBALL_MISSILE',
    destiny.DSTBALL_MUSHROOM: 'DSTBALL_MUSHROOM',
    destiny.DSTBALL_ORBIT: 'DSTBALL_ORBIT',
    destiny.DSTBALL_RIGID: 'DSTBALL_RIGID',
    destiny.DSTBALL_STOP: 'DSTBALL_STOP',
    destiny.DSTBALL_TROLL: 'DSTBALL_TROLL',
    destiny.DSTBALL_WARP: 'DSTBALL_WARP'
}


def format_bytes(bytes):
    if bytes < KB:
        return "{0:.2f}B".format(bytes)
    elif bytes < MB:
        return "{0:.2f}KB".format(float(bytes) / KB)
    elif bytes < GB:
        return "{0:.2f}MB".format(float(bytes) / MB)
    elif bytes < TB:
        return "{0:.2f}GB".format(float(bytes) / GB)
    return "{0:.2f}TB".format(float(bytes) / TB)


class IterationProgress(object):
    def __init__(self, iteration_count):
        self._iteration_count = iteration_count

    def _write_done(self):
        print(
            "Done! Ran for %d iterations (100.0%%)\n" % self._iteration_count
        )

    def report_progress(self, iterations_done):
        percent_complete = float(iterations_done) * 100.0 / float(self._iteration_count)
        if iterations_done == self._iteration_count:
            self._write_done()
            return
        print(
            "Running iteration %d/%d (%.2f%%)" % (
                iterations_done,
                self._iteration_count, percent_complete
            )
        )


class InitializationProgress(object):
    def __init__(self, initializer):
        self._class_name = initializer.__class__.__name__

    def start(self):
        print("Initializing {}".format(self._class_name))

    def end(self, initialization_seconds):
        print(
            "\rInitialized {} in {}".format(
                self._class_name,
                format_seconds(initialization_seconds)
            )
        )


def format_seconds(seconds):
    if seconds < 0.1:
        return "{0:.4f} ms".format(seconds * 1000.0)
    return "{0:.4f} s".format(seconds)


def _print_duration(title, duration):
    print "{}:".format(title)
    print "\tAverage:", format_seconds(duration.avg)
    print "\tStandard deviation: {0:.4f}".format(duration.stddev)
    print "\tmin:", format_seconds(duration.min) if duration.min is not None else "N/A"
    print "\tmax:", format_seconds(duration.max) if duration.max is not None else "N/A"


def _print_network_stats(title, stats):
    print "{}:".format(title)
    print "\tCount:", len(stats.bytes_list)
    print "\tTotal bytes:", format_bytes(sum(stats.bytes_list))
    print "\tAverage bytes:", format_bytes(stats.avg)
    print "\tStandard deviation: {0:.4f}".format(stats.stddev)
    print "\tmin:", format_bytes(stats.min) if stats.min is not None else "N/A"
    print "\tmax:", format_bytes(stats.max) if stats.max is not None else "N/A"


def print_results(results):
    _print_duration("Iteration duration", results.iteration_duration)
    _print_duration("Policy duration", results.policy_duration)
    _print_duration("Tick duration", results.tick_duration)
    _print_duration("Evolve duration", results.evolve_duration)
    _print_duration("Send batch duration", results.send_batch_duration)
    _print_network_stats("Singlecasts", results.singlecast_stats)
    _print_network_stats("Narrowcasts", results.narrowcast_stats)


def print_ball_state(ball):
    print "ball ID: {}".format(ball.id)
    print "bubble: {}".format(ball.newBubbleId)
    print "pos: (%0.4f, %0.4f, %0.4f)" % (ball.x, ball.y, ball.z)
    print "mode:", MODE_TO_STRING.get(ball.mode, "UNKNOWN")
    if ball.mode in (destiny.DSTBALL_WARP, destiny.DSTBALL_GOTO):
        m_from_destination = geo2.Vec3DistanceD(
            (ball.x, ball.y, ball.z),
            (ball.gotoX, ball.gotoY, ball.gotoZ)
        )
        km_from_destination = m_from_destination * 0.001
        print "Km from destination:", km_from_destination


def print_starting_benchmark(benchmark_name, iterations):
    print "Running benchmark", benchmark_name, "for", iterations, "iterations."
