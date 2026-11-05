#!/usr/bin/env python3
"""libtdepkcore against the fake daemon: signals and failures apt can't produce on demand."""
import os
import signal
import subprocess
import sys
import tempfile
import time
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
FAKE = os.path.join(HERE, "..", "fake_pk_daemon.py")
CLI = os.environ.get("PKCORE_CLI", "/build/tree/tests/integration/pkcore-cli")


class Bus:
    """A private bus and a fake daemon on it."""

    def __init__(self, legacy=False):
        self.bus = subprocess.Popen(
            ["dbus-daemon", "--session", "--nofork", "--print-address"],
            stdout=subprocess.PIPE, text=True)
        self.address = self.bus.stdout.readline().strip()
        self.env = dict(os.environ, DBUS_SYSTEM_BUS_ADDRESS=self.address)
        self.logfile = tempfile.NamedTemporaryFile(prefix="fakepk", delete=False).name
        cmd = [sys.executable, FAKE, "--log", self.logfile]
        if legacy:
            cmd.append("--legacy-signals")
        self.daemon = subprocess.Popen(cmd, env=self.env, stdout=subprocess.PIPE, text=True)
        self.daemon.stdout.readline()
        time.sleep(0.2)

    def cli(self, *args, timeout=20):
        run = subprocess.run([CLI, *args], env=self.env, capture_output=True, text=True,
                             timeout=timeout, check=False)
        return run.returncode, [line.split("\t") for line in run.stdout.splitlines()]

    def log(self):
        with open(self.logfile, encoding="utf-8") as handle:
            return handle.read()

    def stop(self):
        for proc in (self.daemon, self.bus):
            if proc.poll() is None:
                proc.send_signal(signal.SIGTERM)
                proc.wait(timeout=5)
        os.unlink(self.logfile)


def lines(output, kind):
    return [line for line in output if line[0] == kind]


FAKE_ID = "fake-{};1.0;amd64;fake-main"


class BaseCase(unittest.TestCase):
    legacy = False

    def setUp(self):
        self.bus = Bus(legacy=self.legacy)
        self.addCleanup(self.bus.stop)

    def run_ok(self, *args):
        code, output = self.bus.cli(*args)
        self.assertEqual(code, 0, output)
        return output


class Queries(BaseCase):
    def test_daemon_properties(self):
        output = self.run_ok("daemon")
        self.assertIn("backend=fake", output[0])
        self.assertIn("version=1.3.1", output[0])
        self.assertTrue(int(lines(output, "roles")[0][1]) > 0)

    def test_search_returns_every_package(self):
        output = self.run_ok("search-name", "fake")
        self.assertEqual(len(lines(output, "package")), 11)
        self.assertIn("SetHints", self.bus.log())
        self.assertIn("interactive=true", self.bus.log())

    def test_summaries_survive(self):
        output = self.run_ok("search-name", "fake-editor")
        self.assertEqual(lines(output, "package")[0][3], "A plain text editor")

    def test_details_map(self):
        output = self.run_ok("details", "fake-editor;3.2;amd64;fake-main")
        row = lines(output, "details")[0]
        self.assertEqual(row[1], "fake-editor;3.2;amd64;fake-main")
        self.assertEqual(row[2], "13")
        self.assertEqual(row[3], "GPL-2.0-or-later")
        self.assertEqual(row[4], "12345678")

    def test_files(self):
        output = self.run_ok("files", FAKE_ID.format("editor"))
        self.assertIn("/usr/bin/fake", lines(output, "files")[0][2])

    def test_update_details(self):
        output = self.run_ok("update-detail", "fake-kernel;6.1.2;amd64;fake-main")
        row = lines(output, "update")[0]
        self.assertEqual(row[2], "4")
        self.assertIn("fake-kernel;0.9", row[3])
        self.assertIn("CVE-2026-0001", row[4])
        self.assertIn("fixes a problem", row[5])

    def test_updates(self):
        output = self.run_ok("updates")
        self.assertEqual(len(lines(output, "package")), 8)

    def test_repos(self):
        output = self.run_ok("repos")
        self.assertEqual([r[2] for r in lines(output, "repo")], ["1", "1", "0"])

    def test_history(self):
        output = self.run_ok("history", "10")
        rows = lines(output, "history")
        self.assertEqual(len(rows), 4)
        self.assertEqual(rows[1][2], "0")

    def test_empty_search_has_no_results(self):
        output = self.run_ok("search-name", "no-such-package")
        self.assertEqual(lines(output, "package"), [])


class LegacySignals(Queries):
    """The same results through the older one-at-a-time signals."""
    legacy = True


class Changes(BaseCase):
    def test_install_sends_trusted_flag(self):
        self.run_ok("install", FAKE_ID.format("editor"))
        self.assertIn(f"InstallPackages 2 {FAKE_ID.format('editor')}", self.bus.log())

    def test_simulate_sends_simulate_flag(self):
        output = self.run_ok("simulate-install", FAKE_ID.format("deps"))
        self.assertIn("InstallPackages 6", self.bus.log())
        names = [row[2] for row in lines(output, "package")]
        self.assertTrue(any("fake-libfoo" in n for n in names))

    def test_remove_passes_flags(self):
        self.run_ok("remove", FAKE_ID.format("editor"))
        self.assertIn("RemovePackages 2", self.bus.log())
        self.assertIn("True False", self.bus.log())

    def test_progress_and_status_reach_client(self):
        output = self.run_ok("install", FAKE_ID.format("editor"))
        self.assertTrue(lines(output, "progress"))
        self.assertTrue(lines(output, "status"))

    def test_failure_reports_error_and_exit(self):
        code, output = self.bus.cli("install", FAKE_ID.format("fail"))
        self.assertEqual(code, 3)
        self.assertIn("dpkg: error", lines(output, "error")[0][2])
        self.assertEqual(lines(output, "finished")[0][1], "2")

    def test_eula_request_then_accept(self):
        code, output = self.bus.cli("install", FAKE_ID.format("eula"))
        self.assertEqual(code, 3)
        self.assertEqual(lines(output, "eula")[0][1], "fake-eula-1")
        self.assertEqual(lines(output, "finished")[0][1], "5")

    def test_signature_request(self):
        code, output = self.bus.cli("install", FAKE_ID.format("key"))
        self.assertEqual(code, 3)
        row = lines(output, "signature")[0]
        self.assertEqual(row[2], "ABCD1234")
        self.assertIn("0123 4567", row[3])
        self.assertEqual(lines(output, "finished")[0][1], "4")

    def test_restart_signal(self):
        output = self.run_ok("install", FAKE_ID.format("restart"))
        self.assertEqual(lines(output, "restart")[0][1], "4")

    def test_untrusted_needs_flag(self):
        code, output = self.bus.cli("install", FAKE_ID.format("untrust"))
        self.assertEqual(code, 3)
        self.assertEqual(lines(output, "finished")[0][1], "8")

    def test_daemon_crash_finishes_transaction(self):
        code, output = self.bus.cli("install", FAKE_ID.format("crash"))
        self.assertEqual(code, 3)
        self.assertIn("stopped unexpectedly", lines(output, "error")[0][2])
        self.assertEqual(len(lines(output, "finished")), 1)

    def test_repo_enable(self):
        self.run_ok("repo-enable", "fake-backports", "1")
        self.assertIn("RepoEnable fake-backports True", self.bus.log())
        output = self.run_ok("repos")
        self.assertEqual(lines(output, "repo")[2][2], "1")

    def test_repo_enable_refused(self):
        code, output = self.bus.cli("repo-enable", "fake-security", "0")
        self.assertEqual(code, 3)
        self.assertIn("protected", lines(output, "error")[0][2])


class NoDaemon(unittest.TestCase):
    def test_no_service_on_bus_is_a_clean_failure(self):
        bus = subprocess.Popen(["dbus-daemon", "--session", "--nofork", "--print-address"],
                               stdout=subprocess.PIPE, text=True)
        self.addCleanup(bus.wait)
        self.addCleanup(bus.terminate)
        env = dict(os.environ, DBUS_SYSTEM_BUS_ADDRESS=bus.stdout.readline().strip())
        run = subprocess.run([CLI, "search-name", "x"], env=env, capture_output=True,
                             text=True, timeout=20, check=False)
        self.assertEqual(run.returncode, 3)
        self.assertIn("error\t", run.stdout)
        self.assertEqual(run.stdout.count("finished\t"), 1)


if __name__ == "__main__":
    unittest.main(verbosity=1)
