# SCOUT: Systematic Context Observer for Unit Telemetry

**Version 0.1.0**

You see this guide because you ran `scout` with no manifest or `scout --help`, or because the
manifest that you named was not found.

## Command Line

| Command                                         | Result                                                             | Exit   |
| :---------------------------------------------- | :----------------------------------------------------------------- | :----- |
| `scout`                                         | Prints this guide.                                                 | 0      |
| `scout --help`, `scout <manifest> --help`       | Prints this guide.                                                 | 0      |
| `scout --version`, `scout <manifest> --version` | Prints `scout 0.1.0 (<commit>)`.                                   | 0      |
| `scout <manifest>`, the file does not exist     | Prints this guide, then `scout: not found: <file name>` on stderr. | 1      |
| `scout <manifest>`                              | Runs the manifest (see The Run).                                   | 0 or 1 |
| `scout <manifest> <other>`                      | Prints `scout: <other>: unknown flag` on stderr.                   | 1      |

The manifest path resolves against the working folder. A symbolic link resolves to its target. On a
terminal, scout clears the screen first. On a terminal, a successful run prints `scout: Fine.`.

## The Run

Scout does these steps in order:

1. Scout parses and validates the manifest. A failure prints
   `scout: <path>:<line> (<column>): <rule>`.
2. Scout runs each row of each `## toolchain` table, in order.
3. When the manifest has a `## run` table, scout runs the epochs and writes the report.

A manifest with no `## run` table runs the toolchain only. It writes no report.

## Manifest

The manifest is a markdown file. The `run` table sets the epochs. Each row is one key.

| key      | type   | meaning                                                 |
| :------- | :----- | :------------------------------------------------------ |
| epochs   | int    | The number of epochs. Each epoch runs the command once. |
| command  | string | The program to run, relative to the manifest.           |
| samples  | string | The file the target writes, relative to the manifest.   |
| output   | string | The report file, relative to the manifest.              |
| baseline | string | An earlier report to compare with. Empty for none.      |

Every `run` path is relative to the manifest. Scout also reads the `## toolchain` table (see
Toolchain) and the `## sample` table (see Sampling). The other tables of the manifest belong to the
target. Scout does not read them.

## Sampling

The `## sample` table turns on kernel and user stack sampling for each epoch. Each row is one key.

| key        | type | meaning                                                                                        |
| :--------- | :--- | :--------------------------------------------------------------------------------------------- |
| intervalMs | int  | The sampling period, in ms. The least value is 1.                                              |
| stackDepth | int  | The most frames that each stack records. macOS: 2 to 128. Linux: 1 to 127. Windows: 1 to 192.  |

Without a `## sample` table, scout samples nothing.

Sampling needs elevated rights. Run scout with `sudo` on macOS and Linux. On Windows, run scout from
an administrator console. When the session cannot start, the run stops with
`scout: <name>: <reason>`. For example, on macOS the name is a sysctl name or `kdebug<number>`, on Linux `perf_event_open`, and on Windows the failed API name.

Scout samples the processes that its command starts, and their children. Scout does not sample
itself. On macOS, scout finds the processes every 100 ms. A process that starts and exits between two looks is not sampled.

A sampling run is its own scout run. The sampler adds load. Thus do not read the `epoch`, `user`
and `system` rows of a sampling run as timings.

The report holds two more tables for sampling (see Report). The key of a row is the function name
of the frame. A frame with no symbol uses the name of its image file. Each character outside
`A-Z a-z 0-9 : -` becomes `-`. The value is the number of samples whose stack holds the frame, in
one epoch. The unit is `count`. A frame with no image (the frame of a process that exited before its symbols were read, or of an anonymous memory region) is not counted. A `dropped` row counts the losses that the kernel reported because its buffer was full. On Linux each table counts its own losses. On macOS and Windows all losses go to `## SAMPLE`; on macOS each count is one loss report, not one record.

## Toolchain

The `## toolchain` table lists the commands that scout runs before the epochs. It has these columns:

| column  | meaning                                                                               |
| :------ | :------------------------------------------------------------------------------------ |
| command | The program to run, verbatim.                                                         |
| flag    | The arguments. Scout splits them on white space. A blank cell runs the command alone. |

A header without both columns fails:
`scout: <path>:<line> (toolchain): toolchain column not declared`.

A span in double quotes stays one argument. Scout removes the quote marks.

Each row runs as one child process. The row runs in the folder that you call scout from. Scout adds
no shell and no path search of its own. The environment is yours. Thus call scout from the folder
that the rows assume, for example the project root. The output of the row streams to stdout.

A row that cannot start, or exits with a code other than 0, stops the run:
`scout: <command> <flag>: toolchain command failed`. No later row and no epoch runs.

No flag selects rows. Every row runs.

## Epochs

For each epoch, scout does these steps:

1. Scout deletes the samples file.
2. Scout runs `<command> <manifest path>`. The working folder is the manifest folder.
3. Scout waits for the command to exit.

A command that exits with a code other than 0 stops the run:
`scout: <command path>: command failed`. A samples file that is missing after the epoch stops the
run: `scout: not found: <samples path>`.

## Samples File

The target writes the samples file with `jam::Profiler::writeSamples`.

The file has one table of zone rows. Its columns are `key | type | value | calls | unit | bins`.

| column | meaning                                                          |
| :----- | :--------------------------------------------------------------- |
| value  | The sum of the durations of the zone in the epoch, in ns.        |
| calls  | The sum of the bins.                                             |
| unit   | `ns`.                                                            |
| bins   | One count for each bin: 64 bins for each octave, and 40 octaves. |

A `dropped` row follows the zone rows. Its unit is `count`. A `run` table follows. It holds
`buildType`, `Debug` or `Release`.

## Report

A `baseline` that names a missing file stops the run before the epochs:
`scout: not found: <baseline path>`.

Scout reads the baseline after the epochs and before it writes the report. Thus a manifest whose
`baseline` names its own `output` compares each run with the run before it.

Every row of a report table holds one sample for each epoch. A row that an epoch's samples file does
not hold adds 0 for that epoch.

Scout writes the report to `output`, relative to the manifest. Scout creates the folder of `output`
when it does not exist. A report that cannot be written stops the run:
`scout: <report path>: cannot write output`.

The report starts with front matter. It holds `date`, the time that scout writes the report, as
`DD-MM-YYYY HH:MM`.

The report has these tables:

| table             | meaning                                                                                              |
| :---------------- | :--------------------------------------------------------------------------------------------------- |
| `## <TYPE>`       | One row for each zone. The value is the median time of one epoch.                                    |
| `## <TYPE>_CALLS` | One row for each zone. The value is the median time of one call, from the merged bins of all epochs. |
| `## RUN`          | The `buildType` of the target.                                                                       |
| `## SAMPLE`       | With `## sample` only. One row for each frame, from the timer samples (see Sampling).                |
| `## CSWITCH`      | With `## sample` only. One row for each frame, from the stacks at each context switch.               |

Each row of a zone table has these columns:

| column       | meaning                                       |
| :----------- | :-------------------------------------------- |
| key          | The name of the row.                          |
| type         | The type of the value.                        |
| value        | The median of the samples.                    |
| errorPercent | The error of the median, in percent.          |
| min          | The smallest sample.                          |
| max          | The largest sample.                           |
| calls        | The number of calls of the zone in one epoch. |
| unit         | The unit of the samples, for example `ns`.    |
| samples      | The sample of each epoch.                     |

The samples file carries `bins` in each row, not the samples of the calls. A bin is one log bin of
the call time. A row has 64 bins for each octave, and 40 octaves. Each bin holds the count of the
calls in that bin.

The `_CALLS` table has a `bins` column instead of `samples`, and one more column, `percentile`:

| column     | meaning                                                 |
| :--------- | :------------------------------------------------------ |
| bins       | The count of calls in each bin, merged from all epochs. |
| percentile | The 99.9th percentile of the merged bins.               |

In the `_CALLS` table, `value`, `errorPercent`, `min`, `max` and `percentile` come from the merged
bins of all epochs. The `calls` column is the number of calls of all epochs.

The `## <TYPE>` table also has these rows:

| row     | meaning                                                                                        |
| :------ | :--------------------------------------------------------------------------------------------- |
| epoch   | The wall time of one epoch.                                                                    |
| user    | The user CPU time of the child, in each epoch.                                                 |
| system  | The system CPU time of the child, in each epoch.                                               |
| dropped | The number of entries that the target lost, because a thread had no slot or its ring was full. |

On Windows, `user` and `system` cover every process of scout's job, also the processes that exited.

With a baseline, each row also has these columns:

| column      | meaning                                   |
| :---------- | :---------------------------------------- |
| baseline    | The median of the baseline.               |
| change      | The change from the baseline.             |
| probability | The probability that the change is noise. |

The report has one median mermaid xychart for each unit. It also has one distribution xychart for
each zone of the `_CALLS` table. The chart shows the calls in each octave. The x axis is the lower
edge of the octave, in ns.

## Exit Codes

| code | meaning                                                                                                                                                                                 |
| :--- | :-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0    | Help, version, or a run where every step passed.                                                                                                                                        |
| 1    | A missing manifest, an unknown flag, a validation failure, a toolchain failure, a command failure, a sampling session that cannot start, a missing baseline file, a missing samples file, or a report that cannot be written. |

A failure prints `scout: <message>` on stderr. A validation message can hold more than one line.

## Example

```
## run

+----------+--------+------------------------------------------------------------------------------+
| key      | type   | value                                                                        |
+==========+========+==============================================================================+
| epochs   | int    | 11                                                                           |
+----------+--------+------------------------------------------------------------------------------+
| command  | string | ../Builds/Replay/EVE_artefacts/Release/Standalone/EVE.app/Contents/MacOS/EVE |
+----------+--------+------------------------------------------------------------------------------+
| samples  | string | replay-samples.md                                                            |
+----------+--------+------------------------------------------------------------------------------+
| output   | string | replay-report.md                                                             |
+----------+--------+------------------------------------------------------------------------------+
| baseline | string |                                                                              |
+----------+--------+------------------------------------------------------------------------------+

## profiler

+-------+--------+--------------------+
| key   | type   | value              |
+=======+========+====================+
| shell | string | /bin/zsh           |
+-------+--------+--------------------+
| args  | string | -c "seq 1 1000000" |
+-------+--------+--------------------+

## toolchain

+---------+--------------------------------------------------------------------------------------------------------------+
| command | flag                                                                                                         |
+=========+==============================================================================================================+
| cast    | ../jam/cast/spell.md                                                                                         |
+---------+--------------------------------------------------------------------------------------------------------------+
| cmake   | -S . -B Builds/Replay -G Ninja -DCMAKE_BUILD_TYPE=Release -DCAST_SIGN=OFF -DCAST_PROFILE=ON -DCAST_REPLAY=ON |
+---------+--------------------------------------------------------------------------------------------------------------+
| ninja   | -C Builds/Replay                                                                                             |
+---------+--------------------------------------------------------------------------------------------------------------+
```

The `profiler` table belongs to the target. EVE reads it. Scout does not read it.

These rows assume that you call scout from the EVE project root.
