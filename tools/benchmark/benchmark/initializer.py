import abc
import random
from time import time

from benchmark.util import get_random_vector


class Initializer(object):
    __metaclass__ = abc.ABCMeta

    def __init__(self, server):
        self._server = server

    @abc.abstractmethod
    def _initialize(self):
        pass

    def initialize(self):
        start_time = time()
        self._initialize()
        end_time = time()
        return end_time - start_time


class EmptyParkInitializer(Initializer):
    def _initialize(self):
        pass


class OverlappingBallsInitializer(Initializer):
    def __init__(self, server, count=10000):
        super(OverlappingBallsInitializer, self).__init__(server)
        self._count = count

    def _initialize(self):
        for i in range(self._count):
            self._server.add_interactive()


class ClusterOfBallsInitializer(Initializer):
    """
    Generates balls within a sphere
    with a radius of clusterRadiusMeters.
    Expect balls to cluster together more
    around the center than near the edge.

    There is nothing preventing generated
    balls from overlapping each other.
    """
    def __init__(self, server, count=10000, cluster_radius_km=1000, ball_radius_meters=30):
        super(ClusterOfBallsInitializer, self).__init__(server)
        self._count = count
        self._cluster_radius_meters = cluster_radius_km * 1000
        self._ball_radius_meters = ball_radius_meters

        if ball_radius_meters > self._cluster_radius_meters:
            raise RuntimeError("Cluster radius does not fit one ball.")

    def _initialize(self):
        for i in range(self._count):
            lower_bound = self._ball_radius_meters
            upper_bound = self._cluster_radius_meters - self._ball_radius_meters

            metersFromCenter = random.randint(lower_bound, upper_bound)
            x, y, z = get_random_vector(metersFromCenter)
            self._server.add_interactive(x=x, y=y, z=z, radius=self._ball_radius_meters)


class EnclosingCubeInitializer(Initializer):
    """
    Creates a cube of boxes enclosing center with internal
    dimensions of width x width x width.
    The cubes sides are aligned with the parks x y and z axis.
    """
    def __init__(self, server, center=(0.0, 0.0, 0.0), width=1000):
        super(EnclosingCubeInitializer, self).__init__(server)
        self._center = center
        self._width = width

    def _initialize(self):
        x, y, z = self._center
        parent_ball = self._server.add_ball(
            x=x, y=y, z=z,
            radius=self._width,
            is_free=False,
            is_massive=False,  # The parent ball is not massive, but the mini-boxes are.
        )
        wall_thickness = self._width * 0.10
        wall_width = self._width + wall_thickness * 2

        # When the box gets created, it's corner is at the "corner offset position"
        # From there, it grows along the specified x, y and z axis, which in our
        # case are all positive. Therefore, the negative corner is further away from the
        # center, since in that direction we need to account for the thickness of the wall,
        # whereas when the corner is along a positive axis, the wall grows outside from the center
        # of the box.
        corner_offset_neg = -(self._width / 2.0) - wall_thickness
        corner_offset_pos = self._width / 2.0

        # X => Right
        # Y => Up
        # Z => Forward

        # back
        parent_ball.AddMiniBox(
            corner_offset_neg, corner_offset_neg, corner_offset_neg,
            wall_width, 0, 0,
            0, wall_width, 0,
            0, 0, wall_thickness
        )

        # front
        parent_ball.AddMiniBox(
            corner_offset_neg, corner_offset_neg, corner_offset_pos,
            wall_width, 0, 0,
            0, wall_width, 0,
            0, 0, wall_thickness
        )

        # left
        parent_ball.AddMiniBox(
            corner_offset_neg, corner_offset_neg, corner_offset_neg,
            wall_thickness, 0, 0,
            0, wall_width, 0,
            0, 0, wall_width
        )

        # right
        parent_ball.AddMiniBox(
            corner_offset_pos, corner_offset_neg, corner_offset_neg,
            wall_thickness, 0, 0,
            0, wall_width, 0,
            0, 0, wall_width
        )

        # bottom
        parent_ball.AddMiniBox(
            corner_offset_neg, corner_offset_neg, corner_offset_neg,
            wall_width, 0, 0,
            0, wall_thickness, 0,
            0, 0, wall_width
        )

        # top
        parent_ball.AddMiniBox(
            corner_offset_neg, corner_offset_pos, corner_offset_neg,
            wall_width, 0, 0,
            0, wall_thickness, 0,
            0, 0, wall_width
        )
