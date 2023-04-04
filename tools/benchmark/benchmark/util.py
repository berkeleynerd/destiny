# stddev and mean shamelessly copied from matchmaking package
# to mimimize external dependencies and
# avoid reinventing the wheel.

import math
import random

import geo2


def mean(values):
    """
    Computes the mean for a set of values.
    """
    if len(values):
        return sum(values) / float(len(values))
    return 0


def stddev(values):
    """
    Computes the standard deviation for a set of values.
    """
    if len(values):
        avg = mean(values)
        variance = [(value - avg) ** 2 for value in values]
        return math.sqrt(sum(variance) / float(len(values)))
    return 0


def get_id_for_interactive_ball():
    try:
        return get_id_for_interactive_ball._next_id
    finally:
        get_id_for_interactive_ball._next_id += 1
get_id_for_interactive_ball._next_id = 1


def get_random_direction():
    x = random.random() * 2.0 - 1.0
    y = random.random() * 2.0 - 1.0
    z = random.random() * 2.0 - 1.0
    if (x, y, z) == (0.0, 0.0, 0.0):
        x = 1.0  # (0.0, 0.0, 0.0) is not a direction.
    return x, y, z


def get_random_normal():
    x, y, z = get_random_direction()
    return geo2.Vec3NormalizeD((x, y, z))


def get_random_vector(length):
    x, y, z = get_random_normal()
    return x * length, y * length, z * length
