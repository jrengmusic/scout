#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @struct Validator
 * @brief Column-rule validation for a scout manifest -- the config rules, the
 *        toolchain column rule and the sample range rule.
 *
 * addRules() composes ConfigValidator's seven rules with two more rules:
 * `toolchain` fails a `## toolchain` table whose header row declares no
 * `command` column or no `flag` column. `sample` fails a `## sample` table
 * whose `intervalMs` is below Sampler::minimumIntervalMs, or whose `stackDepth`
 * is outside Sampler::minimumStackDepth and Sampler::maximumStackDepth.
 * getRules() returns the nine rules, built once.
 */
struct Validator : jam::ConfigValidator
{
    /**
     * @brief Adds the rules of ConfigValidator, then `toolchain` and `sample`.
     * @param rules  Rules that receive the entries.
     */
    static void addRules (Rules& rules);

    /**
     * @brief Returns the nine scout rules: the seven config rules, `toolchain` and `sample`.
     *
     * `toolchain` fails a `## toolchain` table whose header row lacks a
     * `command` column or a `flag` column. The failure names the table and
     * the header row.
     * `sample` fails a `## sample` table whose `intervalMs` or `stackDepth`
     * is out of range. The failure names the table and the header row.
     *
     * @return The rules, keyed by rule name, built once.
     */
    const Rules& getRules() const override;
};
