# Copyright © 2023 CCP ehf.

from os.path import abspath, dirname
import sys
import destiny
import unittest


PYTHON_MODULE_PATH = abspath(dirname(dirname(dirname(__file__))))


class TestUsingCorrectModule(unittest.TestCase):
    """
    Since we have packages on the path,
    let's make sure we are executing from the root python folder
    and are using the destiny module from this project.
    """
    def test_using_destiny_from_carbon_module(self):
        self.assertTrue(
            PYTHON_MODULE_PATH in destiny.__file__
            or destiny.__file__ == "destiny\\__init__.py"
        )

    def test_destiny_python_module_is_in_path(self):
        self.assertIn(PYTHON_MODULE_PATH, sys.path)
