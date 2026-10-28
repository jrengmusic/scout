# scout

Systematic Context Observer for Unit Telemetry.

## Command Line

```
scout <manifest>.md
```

Scout takes one argument: the path of the manifest. Scout runs the `command` of the manifest once for each epoch. The one argument of the command is the manifest path. The working folder of the command is the manifest folder. Scout reads the `samples` file that the command writes in each epoch. Scout writes the report to the `output` file.

## Manifest

The manifest is a markdown file. The `run` table sets the run. Each row is one key.

| key      | type   | meaning                                                  |
|:---------|:-------|:---------------------------------------------------------|
| epochs   | int    | The number of epochs. Each epoch runs the command once.  |
| command  | string | The program to run, relative to the manifest.            |
| samples  | string | The file the target writes, relative to the manifest.    |
| output   | string | The report file, relative to the manifest.               |
| baseline | string | An earlier report to compare with. Empty for none.       |

The other tables of the manifest belong to the target. Scout does not read them.

## Report

The report has these tables:

| table             | meaning                                                           |
|:------------------|:------------------------------------------------------------------|
| `## <TYPE>`       | One row for each zone. The value is the median time of one epoch. |
| `## <TYPE>_CALLS` | One row for each zone. The value is the median time of one call, from the merged bins of all epochs. |
| `## RUN`          | The `buildType` of the target.                                    |

Each row of a zone table has these columns:

| column       | meaning                                                          |
|:-------------|:-----------------------------------------------------------------|
| key          | The name of the row.                                             |
| type         | The type of the value.                                           |
| value        | The median of the samples.                                       |
| errorPercent | The error of the median, in percent.                             |
| min          | The smallest sample.                                             |
| max          | The largest sample.                                              |
| calls        | The number of calls of the zone in one epoch.                    |
| unit         | The unit of the samples, for example `ns`.                       |
| samples      | The sample of each epoch.                                        |

The samples file carries `bins` in each row, not the samples of the calls. A bin is one log bin of the call time.
A row has 64 bins for each octave, and 40 octaves. Each bin holds the count of the calls in that bin.

The `_CALLS` table has a `bins` column instead of `samples`, and one more column, `percentile`:

| column     | meaning                                                                  |
|:-----------|:-------------------------------------------------------------------------|
| bins       | The count of calls in each bin, merged from all epochs.                  |
| percentile | The 99.9th percentile of the merged bins.                                |

In the `_CALLS` table, `value`, `errorPercent`, `min`, `max` and `percentile` come from the merged bins of all epochs.
The `calls` column is the number of calls of all epochs.

The `## <TYPE>` table also has these rows:

| row     | meaning                                                                                        |
|:--------|:-----------------------------------------------------------------------------------------------|
| epoch   | The wall time of one epoch.                                                                    |
| user    | The user CPU time of the child, in each epoch. Only on macOS and Linux.                        |
| system  | The system CPU time of the child, in each epoch. Only on macOS and Linux.                      |
| dropped | The number of entries that the target lost, because a thread had no slot or its ring was full. |

With a baseline, each row also has these columns:

| column      | meaning                                   |
|:------------|:------------------------------------------|
| baseline    | The median of the baseline.               |
| change      | The change from the baseline.             |
| probability | The probability that the change is noise. |

The report has one median mermaid xychart for each unit.
It also has one distribution xychart for each zone of the `_CALLS` table. The chart shows the calls in each octave.
The x axis is the lower edge of the octave, in ns.

## Example

## run

+----------+--------+--------------------------------------------------------------------------------+
| key      | type   | value                                                                          |
+==========+========+================================================================================+
| epochs   | int    | 11                                                                             |
+----------+--------+--------------------------------------------------------------------------------+
| command  | string | ../Builds/Replay/EVE_artefacts/Release/Standalone/EVE.app/Contents/MacOS/EVE   |
+----------+--------+--------------------------------------------------------------------------------+
| samples  | string | replay-samples.md                                                              |
+----------+--------+--------------------------------------------------------------------------------+
| output   | string | replay-report.md                                                               |
+----------+--------+--------------------------------------------------------------------------------+
| baseline | string |                                                                                |
+----------+--------+--------------------------------------------------------------------------------+

## profiler

+-------+--------+--------------------+
| key   | type   | value              |
+=======+========+====================+
| shell | string | /bin/zsh           |
+-------+--------+--------------------+
| args  | string | -c "seq 1 1000000" |
+-------+--------+--------------------+
