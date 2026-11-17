# Clean

Clean is a C++ command line tool for organizing files. The existing `sort` command organizes files by type, date, or size. The `listen` and `execute` commands form a small learning organizer: Clean watches how you move files between folders, groups those examples with K-means, and suggests similar moves.

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

Clean writes its data to `.clean/state.txt` and a graph to `.clean/graph.svg` in that directory. Open the SVG in a browser. Each dot is a recorded file change; larger dots are moves. Horizontal position represents file size on a logarithmic scale, vertical position represents an extension bucket, and color shows the nearest learned cluster. Hover over a dot to see the change and file path. Cluster labels show their learned destination folder and example count.

Moves between subdirectories are training examples. Clean matches a move by file size, modification time, and extension, using the filename to resolve matches when it can. Ambiguous moves stay recorded as a deletion and addition. K-means uses file extension and size to group examples, then assigns each group its most common destination. At least two examples and a clear majority are needed before Clean suggests a move.

## Review suggestions

```sh
clean execute
```

Stop `listen` before running `execute` in another shell. Clean asks `Move X to Y? [y/n/q]` for each suggestion. `y` moves it, `n` declines that suggestion for that file, and `q` stops. It skips existing destination files and does not overwrite them. Accepted moves become new examples. Rejected suggestions are saved so they are not repeatedly asked.

Once you have accepted at least three suggestions, with at least three accepts per rejection, you can opt into automatic moves:

```sh
clean execute --auto
```

Automatic mode applies the same confidence and collision checks. The learned model is intentionally simple; inspect the graph and suggestions before enabling it. Remove `.clean` to reset the learning history for that directory.

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

The test uses a temporary directory and checks learning, suggestions, accept and reject behavior, automatic mode, collision handling, graph output, and the existing sort command.
