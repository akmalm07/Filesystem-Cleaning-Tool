"""End-to-end test for the file listener and its learned suggestions."""

import subprocess
import sys
import tempfile
from pathlib import Path
from xml.etree import ElementTree


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
        assert "Accepted: 3, rejected: 1." in result.stdout
        assert 'A "Inbox/auto.txt"' not in (root / ".clean" / "state.txt").read_text()

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


def test_filename_suggestions(executable):
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        inbox = root / "Inbox"
        inbox.mkdir()
        accounting = root / "Accounting"
        coursework = root / "Coursework"
        accounting.mkdir()
        coursework.mkdir()

        examples = {
            "INVOICE.March,2025.PDF": accounting,
            "invoice_April-2025.pdf": accounting,
            "Lesson Plan (Spring).pdf": coursework,
            "lesson-plan.winter.pdf": coursework,
        }
        for i, name in enumerate(examples):
            (inbox / name).write_text("sample\n" * (i + 1))
        result = run(executable, root, "listen", "--once")
        assert result.returncode == 0, result.stdout + result.stderr
        state = root / ".clean" / "state.txt"
        # The original snapshot format can still be loaded and updated.
        state.write_text(state.read_text().replace("CLEAN2", "CLEAN1", 1))
        for name, folder in examples.items():
            (inbox / name).rename(folder / name)
        result = run(executable, root, "listen", "--once")
        assert "Learned 4 moves." in result.stdout, result.stdout + result.stderr
        assert state.read_text().startswith("CLEAN2")
        state.write_text(state.read_text().replace("CLEAN2", "CLEAN1", 1))

        approvals = {
            "Invoice May2026.PdF": accounting,
            "Invoice June_2026.pdf": accounting,
            "Lesson Plan,Fall.pdf": coursework,
            "Lesson.Plan Summer.pdf": coursework,
        }
        for name in (*approvals, "Unrelated.pdf", "Invoice May.txt"):
            (inbox / name).write_text("new file")
        result = run(executable, root, "execute", answer="y\n" * len(approvals))
        assert result.returncode == 0, result.stdout + result.stderr
        assert result.stdout.count("Reason: Similar filenames") == len(approvals), result.stdout
        for name, folder in approvals.items():
            assert (folder / name).is_file(), result.stdout
        assert (inbox / "Unrelated.pdf").is_file()
        assert (inbox / "Invoice May.txt").is_file()
        assert state.read_text().startswith("CLEAN2")
        assert sum(line.startswith("A ") for line in state.read_text().splitlines()) == 4

        (inbox / "Invoice,July 2026.pdf").write_text("new file")
        (inbox / "Lesson_Plan,Autumn.pdf").write_text("new file")
        result = run(executable, root, "execute", answer="n\nn\n")
        assert result.returncode == 0, result.stdout + result.stderr
        assert result.stdout.count("Reason: Matches approved filename patterns") == 2, result.stdout
        assert 'to "Accounting"? [y/n/q]' in result.stdout
        assert 'to "Coursework"? [y/n/q]' in result.stdout
        assert (inbox / "Invoice,July 2026.pdf").is_file()
        assert (inbox / "Lesson_Plan,Autumn.pdf").is_file()
        assert sum(line.startswith("A ") for line in state.read_text().splitlines()) == 4

        svg = ElementTree.parse(root / ".clean" / "suggestions.svg").getroot()
        labels = list(svg.itertext())
        assert "Cluster 2" in labels
        assert any("Invoice,July 2026.pdf to Accounting: Matches approved" in label for label in labels)
        assert any("Lesson_Plan,Autumn.pdf to Coursework: Matches approved" in label for label in labels)
        result = run(executable, root, "execute")
        assert "Suggested 0, moved 0." in result.stdout, result.stdout

    print("Filename suggestion integration test passed")


if __name__ == "__main__":
    main()
    test_filename_suggestions(Path(sys.argv[1]).resolve())
