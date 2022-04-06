#!/usr/bin/env python3

from __future__ import print_function

import argparse
import os
import subprocess
import sys


def parse_test_output(output):
    """Parse a standard output from a test."""
    matchstring = "[com.apple.mfi.HomeKit.Core.Test:Report] "
    passed_tests_prefix = "  Passed tests: "
    failed_tests_prefix = "  Failed tests: "
    missing_tests_prefix = "  Missing tests: "
    passed_test_count = None
    failed_test_count = None
    missing_test_count = None
    report_lines = []
    for aline in output.split("\n"):
        index = aline.find(matchstring)
        if index < 0:
            continue
        stripped = aline[index + len(matchstring) :]
        report_lines.append(stripped)
        if stripped.startswith(passed_tests_prefix):
            passed_test_count = int(stripped[len(passed_tests_prefix) :])
        elif stripped.startswith(failed_tests_prefix):
            failed_test_count = int(stripped[len(failed_tests_prefix) :])
        elif stripped.startswith(missing_tests_prefix):
            missing_test_count = int(stripped[len(missing_tests_prefix) :])
    if (
        passed_test_count is not None
        and failed_test_count is not None
        and missing_test_count is not None
    ):
        return (passed_test_count, failed_test_count, missing_test_count, report_lines)
    return None


def getstatusoutput(cmd):
    """Gets status and output tuple from a command execution"""
    if sys.version_info.major == 3:
        return subprocess.getstatusoutput(cmd)
    try:
        output = subprocess.check_output(
            cmd, shell=True, universal_newlines=True, stderr=subprocess.STDOUT
        )
        status = 0
    except subprocess.CalledProcessError as err:
        output = err.output
        status = err.returncode
    output.rstrip()
    return status, output


def run_single_test(test_cmd, result_db):
    """Run a test and add the result to result_db."""
    print(test_cmd)
    status, output = getstatusoutput(test_cmd)
    parsed_result = parse_test_output(output)
    print(output)
    if parsed_result is None:
        if status == 0:
            result_db.passed_non_framework_suite_count += 1
        else:
            result_db.failed_non_framework_suite_names.append(test_cmd)
    else:
        (
            passed_test_count,
            failed_test_count,
            missing_test_count,
            report_lines,
        ) = parsed_result
        result_db.passed_framework_test_count += passed_test_count
        result_db.failed_framework_test_count += failed_test_count
        result_db.missing_framework_test_count += missing_test_count
        if status == 0:
            result_db.passed_framework_suite_count += 1
        else:
            result_db.failed_framework_suite_count += 1
            result_db.failed_suite_reports.extend(report_lines)


class ResultDatabase(object):
    def __init__(self):
        self.passed_framework_test_count = 0
        self.failed_framework_test_count = 0
        self.missing_framework_test_count = 0
        self.passed_framework_suite_count = 0
        self.failed_framework_suite_count = 0
        self.passed_non_framework_suite_count = 0
        self.failed_non_framework_suite_names = []
        self.failed_suite_reports = []

    def report(self):
        if len(self.failed_non_framework_suite_names) > 0:
            print("*** The following non-framework tests have failed ***")
            for name in self.failed_non_framework_suite_names:
                print(name)
            print("*** End of failed non-framework tests ***")
        if len(self.failed_suite_reports) > 0:
            print("*** Reports from failed test suites follows ***")
            for aline in self.failed_suite_reports:
                print(aline)
            print("*** End of reports from failed test suites ***")
        print("****************")
        print("* Test summary *")
        print("****************")
        print("")
        print("***********************")
        print("* Unit Test Framework *")
        print("***********************")
        print(
            "Total test suites:",
            self.passed_framework_suite_count + self.failed_framework_suite_count,
        )
        print("      Passed:", self.passed_framework_suite_count)
        print("      Failed:", self.failed_framework_suite_count)
        print(
            "Total test cases:",
            self.passed_framework_test_count + self.failed_framework_test_count,
        )
        print("      Passed:", self.passed_framework_test_count)
        print("      Failed:", self.failed_framework_test_count)
        print("")
        print("*****************************")
        print("* Non - Unit Test Framework *")
        print("*****************************")
        print(
            "Total test suites:",
            self.passed_non_framework_suite_count
            + len(self.failed_non_framework_suite_names),
        )
        print("      Passed:", self.passed_non_framework_suite_count)
        print("      Failed:", len(self.failed_non_framework_suite_names))
        print("****************")
        print("* End of Tests *")
        print("****************")

    def all_passed(self):
        return (
            len(self.failed_non_framework_suite_names) == 0
            and self.failed_framework_suite_count == 0
        )


def run_tests(test_cmds):
    """Run multiple test commands and generate collective reports.
    Returns True if all test suites succeeded."""
    result_db = ResultDatabase()
    for test_cmd in test_cmds:
        run_single_test(test_cmd, result_db)
        if result_db.missing_framework_test_count > 0:
            # Upon detecting a missing test, abort all tests without summary report.
            return False
    result_db.report()
    return result_db.all_passed()


def run_tests_from_fileio(inputfile):
    """Run test commands from a file object"""
    cmds = []
    while True:
        aline = inputfile.readline()
        if aline == "":
            break
        cmds.append(aline.rstrip())
    return run_tests(cmds)


def run_tests_from_stdin():
    """Run test commands sent to the standard input of this script"""
    return run_tests_from_fileio(sys.stdin)


def run_tests_from_file(filepath):
    """Run tests commands from a file"""
    result = False
    with open(filepath) as inf:
        result = run_tests_from_fileio(inf)
    return result


def main():
    parser = argparse.ArgumentParser(
        description="Run tests and generate collective test reports"
    )
    parser.add_argument(
        "filepath",
        nargs="?",
        default=None,
        help="File which contains line separated list of test commands to execute. "
        + "If not specified, stdin will be used. The specified file will be deleted.",
    )
    args = parser.parse_args()
    if args.filepath is None:
        result = run_tests_from_stdin()
    else:
        result = run_tests_from_file(args.filepath)
    if not result:
        sys.exit(1)


if __name__ == "__main__":
    main()
