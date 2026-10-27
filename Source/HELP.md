# scout

Systematic Context Observer for Unit Telemetry.

## Command Line

```
scout <manifest>.md
```

Scout takes one argument: the path of the manifest. Scout runs the benchmarks the manifest names. Scout writes the report to the `output` file.

## Manifest

The manifest is a markdown file with two tables.

The `run` table sets the run. Each row is one key.

| key      | type    | meaning                                                  |
|:---------|:--------|:---------------------------------------------------------|
| epochs   | int     | The number of epochs each benchmark runs.                |
| seed     | int     | The seed for the random numbers of the run.              |
| output   | string  | The report file. The path is relative to the manifest.   |
| baseline | string  | An earlier report to compare with. Leave empty for none. |

The `benchmark` table names the benchmarks. Each row has one `name`. The name is a registered benchmark.

## Report

The report has one `##` table for each benchmark. Each row is one zone.

| column       | meaning                              |
|:-------------|:-------------------------------------|
| key          | The name of the zone.                |
| type         | The type of the value.               |
| value        | The median time per epoch, in ns.    |
| errorPercent | The error of the median, in percent. |
| min          | The shortest time.                   |
| max          | The longest time.                    |
| calls        | The number of calls of the zone.     |
| samples      | The time of each epoch, in ns.       |

With a baseline, each row also has these columns:

| column      | meaning                                       |
|:------------|:----------------------------------------------|
| baseline    | The median of the baseline.                   |
| change      | The change from the baseline.                 |
| probability | The probability that the change is noise.     |

Each benchmark also has one mermaid xychart.

## Example

## run

+----------+---------+---------------+
| key      | type    | value         |
+==========+=========+===============+
| epochs   | int     | 11            |
+----------+---------+---------------+
| seed     | int     | 0             |
+----------+---------+---------------+
| output   | string  | eve-report.md |
+----------+---------+---------------+
| baseline | string  |               |
+----------+---------+---------------+

## benchmark

+----------+
| name     |
+==========+
| Terminal |
+----------+
