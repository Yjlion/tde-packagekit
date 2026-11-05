#!/usr/bin/env python3
"""libtdepkcore against a real packagekitd and apt, as an unprivileged user."""
import os
import subprocess
import unittest

CLI = os.environ.get("PKCORE_CLI", "/build/tree/tests/integration/pkcore-cli")
SERVICES = ["sudo", "/src/docker/test-services.sh"]


def services(*args):
    subprocess.run(SERVICES + list(args), check=True)


def cli(*args, timeout=180):
    run = subprocess.run([CLI, *args], capture_output=True, text=True, timeout=timeout, check=False)
    return run.returncode, [line.split("\t") for line in run.stdout.splitlines()]


def lines(output, kind):
    return [line for line in output if line[0] == kind]


def installed_version(name):
    run = subprocess.run(["dpkg-query", "-W", "-f", "${Status} ${Version}", name],
                         capture_output=True, text=True, check=False)
    if run.returncode == 0 and run.stdout.startswith("install ok installed"):
        return run.stdout.split()[-1]
    return None


def find_id(name, want_installed=None):
    code, output = cli("resolve", name)
    assert code == 0, output
    for row in lines(output, "package"):
        if row[2].startswith(name + ";"):
            if want_installed is None or (row[1] == "1") == want_installed:
                return row[2]
    raise AssertionError(f"{name} not found in {output}")


def purge(*names):
    subprocess.run(["sudo", "/usr/bin/dpkg", "--purge", *names], capture_output=True, check=False)


class RealDaemon(unittest.TestCase):
    """Tests in this class share the system, so they run in a set order."""

    @classmethod
    def setUpClass(cls):
        purge("tdepk-test-hello", "tdepk-test-app", "tdepk-test-lib", "tdepk-test-local")
        services("polkit-rule", "allow")
        services("use-fixture-repo", "v1")
        services("restart-packagekit")

    def test_01_daemon_is_real(self):
        code, output = cli("daemon")
        self.assertEqual(code, 0)
        self.assertIn("backend=apt", output[0])

    def test_02_search(self):
        code, output = cli("search-name", "tdepk-test")
        self.assertEqual(code, 0)
        names = sorted(row[2].split(";")[0] for row in lines(output, "package"))
        self.assertEqual(names, ["tdepk-test-app", "tdepk-test-hello", "tdepk-test-lib"])

    def test_03_details(self):
        code, output = cli("details", find_id("tdepk-test-hello"))
        self.assertEqual(code, 0)
        self.assertEqual(len(lines(output, "details")), 1)

    def test_04_depends_on(self):
        code, output = cli("depends", find_id("tdepk-test-app"))
        self.assertEqual(code, 0)
        self.assertTrue(any("tdepk-test-lib" in row[2] for row in lines(output, "package")))

    def test_05_simulate_changes_nothing(self):
        code, output = cli("simulate-install", find_id("tdepk-test-app"))
        self.assertEqual(code, 0, output)
        names = [row[2] for row in lines(output, "package")]
        self.assertTrue(any("tdepk-test-lib" in n for n in names), names)
        self.assertIsNone(installed_version("tdepk-test-app"))

    def test_06_install_with_dependency(self):
        code, output = cli("install", find_id("tdepk-test-app"))
        self.assertEqual(code, 0, output)
        self.assertEqual(installed_version("tdepk-test-app"), "1.0")
        self.assertEqual(installed_version("tdepk-test-lib"), "1.0")

    def test_07_files_of_installed(self):
        code, output = cli("files", find_id("tdepk-test-app", want_installed=True))
        self.assertEqual(code, 0, output)
        self.assertIn("/usr/share/doc/tdepk-test-app/README", lines(output, "files")[0][2])

    def test_08_required_by(self):
        code, output = cli("required-by", find_id("tdepk-test-lib", want_installed=True))
        self.assertEqual(code, 0, output)
        self.assertTrue(any("tdepk-test-app" in row[2] for row in lines(output, "package")))

    def test_09_update(self):
        code, output = cli("install", find_id("tdepk-test-hello"))
        self.assertEqual(code, 0, output)
        self.assertEqual(installed_version("tdepk-test-hello"), "1.0")

        services("use-fixture-repo", "v2")
        services("restart-packagekit")
        code, output = cli("updates")
        self.assertEqual(code, 0, output)
        ids = [row[2] for row in lines(output, "package") if row[2].startswith("tdepk-test-hello;")]
        self.assertEqual(len(ids), 1, output)
        self.assertIn(";1.1;", ids[0])

        code, detail = cli("update-detail", ids[0])
        self.assertEqual(code, 0, detail)
        self.assertEqual(len(lines(detail, "update")), 1)

        code, output = cli("simulate-update", ids[0])
        self.assertEqual(code, 0, output)
        self.assertEqual(installed_version("tdepk-test-hello"), "1.0")

        code, output = cli("update", ids[0])
        self.assertEqual(code, 0, output)
        self.assertEqual(installed_version("tdepk-test-hello"), "1.1")

    def test_10_remove_takes_dependents(self):
        installed = find_id("tdepk-test-lib", want_installed=True)
        code, output = cli("simulate-remove", installed)
        self.assertEqual(code, 0, output)
        self.assertTrue(any("tdepk-test-app" in row[2] for row in lines(output, "package")), output)
        self.assertEqual(installed_version("tdepk-test-app"), "1.0")

        code, output = cli("remove", installed)
        self.assertEqual(code, 0, output)
        self.assertIsNone(installed_version("tdepk-test-lib"))
        self.assertIsNone(installed_version("tdepk-test-app"))

    def test_11_install_local_file(self):
        path = "/build/fixture-repo/tdepk-test-local_1.0_all.deb"
        # A local file can't be authenticated, so the client must ask, then retry.
        code, output = cli("install-file", path)
        self.assertEqual(code, 3, output)
        self.assertEqual(lines(output, "finished")[0][1], "8", output)
        self.assertIsNone(installed_version("tdepk-test-local"))

        code, output = cli("install-file-untrusted", path)
        self.assertEqual(code, 0, output)
        self.assertEqual(installed_version("tdepk-test-local"), "1.0")

    def test_12_history_records_changes(self):
        code, output = cli("history", "50")
        self.assertEqual(code, 0, output)
        self.assertTrue(lines(output, "history"), output)

    def test_13_repos_toggle(self):
        code, output = cli("repos")
        self.assertEqual(code, 0, output)
        repo = lines(output, "repo")[0]
        self.assertEqual(repo[2], "1")
        code, output = cli("repo-enable", repo[1], "0")
        self.assertEqual(code, 0, output)
        code, output = cli("repos")
        self.assertEqual(lines(output, "repo")[0][2], "0")
        code, output = cli("repo-enable", repo[1], "1")
        self.assertEqual(code, 0, output)
        code, output = cli("repos")
        self.assertEqual(lines(output, "repo")[0][2], "1")

    def test_14_denied_authorization(self):
        services("polkit-rule", "deny")
        self.addCleanup(services, "polkit-rule", "allow")
        purge("tdepk-test-hello")
        services("use-fixture-repo", "v1")
        services("restart-packagekit")
        code, output = cli("install", find_id("tdepk-test-hello"))
        self.assertNotEqual(code, 0, output)
        self.assertTrue(lines(output, "error"), output)
        # 48 is PK_ERROR_ENUM_NOT_AUTHORIZED (the unit tests check the enum).
        self.assertEqual(lines(output, "error")[0][1], "48", output)
        self.assertIsNone(installed_version("tdepk-test-hello"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
