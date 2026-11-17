"""End-to-end test for the file listener and its learned suggestions."""

import subprocess
import sys
import tempfile
from pathlib import Path


def run(executable, directory, *args, answer=""):
    result = subprocess.run(
        [str(executable), *args],
        cwd=directory,
        input=answer,
        text=True,
        capture_output=True,
        check=False,
    )
    return result


def main():
    executable = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        inbox = root / "Inbox"
        docs = root / "Documents"
        inbox.mkdir()
        docs.mkdir()
        (root / "root.txt").write_text("ignored")
        for name in ("one.txt", "two.txt"):
            (inbox / name).write_text(name)
        (docs / "change.txt").write_text("before")
        (docs / "delete.txt").write_text("remove")

        result = run(executable, root, "listen", "--once")
        assert result.returncode == 0, result.stderr + result.stdout
        state = (root / ".clean" / "state.txt").read_text()
        assert "Inbox/one.txt" in state
        assert "root.txt" not in state

        for name in ("one.txt", "two.txt"):
            (inbox / name).rename(docs / name)
        (docs / "change.txt").write_text("after modification")
        (docs / "delete.txt").unlink()
        (docs / "added.bin").write_text("new")
        result = run(executable, root, "listen", "--once")
        assert result.returncode == 0, result.stderr + result.stdout
        assert "Learned 2 moves" in result.stdout

        assert run(executable, root, "execute", "--auto").returncode != 0

        (inbox / "reject.txt").write_text("reject")
        result = run(executable, root, "execute", answer="n\n")
        assert result.returncode == 0, result.stderr + result.stdout
        assert (inbox / "reject.txt").exists()

        for name in ("approve1.txt", "approve2.txt", "approve3.txt"):
            (inbox / name).write_text(name)
        result = run(executable, root, "execute", answer="y\ny\ny\n")
        assert result.returncode == 0, result.stderr + result.stdout
        for name in ("approve1.txt", "approve2.txt", "approve3.txt"):
            assert (docs / name).exists()
        assert (inbox / "reject.txt").exists()

        (inbox / "auto.txt").write_text("auto")
        result = run(executable, root, "execute", "--auto")
        assert result.returncode == 0, result.stderr + result.stdout
        assert (docs / "auto.txt").exists()

        (inbox / "collision.txt").write_text("source")
        (docs / "collision.txt").write_text("target")
        result = run(executable, root, "execute", "--auto")
        assert result.returncode == 0, result.stderr + result.stdout
        assert (inbox / "collision.txt").read_text() == "source"
        assert (docs / "collision.txt").read_text() == "target"

        graph = (root / ".clean" / "graph.svg").read_text()
        assert "<svg" in graph and "Documents" in graph
        for change in ("moved:", "modified:", "deleted:", "added:"):
            assert change in graph
        assert run(executable, root, "sort", "--help").returncode == 0
        assert run(executable, root, "listen", "--interval", "0").returncode != 0

    print("Listener integration test passed")


if __name__ == "__main__":
    main()
