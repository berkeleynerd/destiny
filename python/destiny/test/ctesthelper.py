# Copyright © 2023 CCP ehf.

import unittest


def _print_suite(test_suite_or_iterable):
    if hasattr(test_suite_or_iterable, '_exception'):
        # noinspection PyProtectedMember
        print(test_suite_or_iterable._exception)
    elif not hasattr(test_suite_or_iterable, '__iter__'):
        print(test_suite_or_iterable.id())
    else:
        for item in test_suite_or_iterable:
            _print_suite(item)


def discover_tests():
    test_suite = unittest.defaultTestLoader.discover('.')
    _print_suite(test_suite)
