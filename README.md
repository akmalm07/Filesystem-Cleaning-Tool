# Clean

Clean is a C++ command line tool for organizing files. The `sort` command organizes files by type, date, or size. The `listen` and `execute` commands learn from how you move files. Clean uses K-means to group file activity and filename patterns, then suggests destinations based on your past moves.

## Demo Video

https://github.com/user-attachments/assets/61352f1b-345f-496e-8c72-0aecf45db212


## Build

Clean uses C++23 and Premake5. From the project root:

```sh
premake5 vs2022
```

Open `cleanCLI.sln` and build the `Clean` project. For a direct GCC build:

```sh
g++ -std=c++23 -Iclean/global -Iclean/include clean/src/*.cpp clean/global/*.cpp -o clean
```

## Learn your folder layout

Run these commands **from the top level directory** that contains the folders you want Clean to watch. `listen` observes regular files inside subdirectories; it ignores files directly in the current directory, symlinks, `.git`, and `.clean`.

```sh
clean listen
```

The first run saves a baseline. Keep it running while you organize files yourself. It scans every two seconds until you press Ctrl+C. Each scan records added, deleted, modified, and moved files. If you prefer manual scans, run `clean listen --once` before and after organizing files. Use `--interval 5` to change the polling interval in seconds.

Clean writes its history to `.clean/state.txt` and exports two SVGs you can open in a browser:

- `.clean/graph.svg` shows recorded file changes by size, extension bucket, and activity cluster.
- `.clean/suggestions.svg` shows filename clusters and the files in them. Blue dots are observed moves, green dots are approved moves, and amber dots are suggestions from the latest execution. Hover over a dot for the file, destination, and suggestion reason when available.

Moves between subdirectories are training examples. Clean matches a move by file size, modification time, and extension, using the filename to resolve matches when it can. Ambiguous moves stay recorded as a deletion and addition.

## Review suggestions

```sh
clean execute
```

Stop `listen` before running `execute`. Clean splits filenames on spaces, periods, commas, dashes, and other punctuation. If you moved `invoice-march.pdf` and `invoice_april.pdf` to `Accounting`, it can suggest the same destination for `Invoice May,2026.pdf`. Clean looks at the exact extension and uses the source folder and size to help rank similar names.

Clean explains each suggestion and asks `Move X to Y? [y/n/q]`. Answering `y` moves the file and adds an approved example to the decision tree for later runs. Answering `n` leaves the file in place and remembers the declined destination. Answering `q` stops the review. Clean skips existing destination files.

Once you have accepted at least three suggestions, with at least three accepts per rejection, you can opt into automatic moves:

```sh
clean execute --auto
```

Automatic mode applies the same confidence and collision checks. Its moves appear in the activity graph. Only manual approvals train the decision tree. Remove `.clean` to reset the learning history for that directory.

See [ARCHITECTURE.md](ARCHITECTURE.md) for how both models, the decision tree, and saved state fit together.

## Static sorting

The original `sort` command is still available:

```sh
clean sort source destination --by-type 1 --by-date 2 --move
clean sort --help
```

Use `clean --help` for the command list and `clean listen --help` or `clean execute --help` for their options.

## Test

Build the executable, then run the integration test with its absolute path:

```sh
python tests/test_listen.py /absolute/path/to/clean
```

The test uses temporary directories to check file change detection, filename matching, approval tree predictions across runs, saved state compatibility, collision handling, and the existing sort command.
