#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @struct Validator
 * @brief Column-rule validation for a scout manifest -- the config rules and
 *        the toolchain column rule.
 *
 * addRules() composes ConfigValidator's seven rules with one more rule:
 * `toolchain` fails a `## toolchain` table whose header row declares no
 * `command` column or no `flag` column. getRules() returns the eight rules,
 * built once.
 */
struct Validator : jam::ConfigValidator
{
    /**
     * @brief Adds the rules of ConfigValidator, then `toolchain`.
     * @param rules  Rules that receive the entries.
     */
    static void addRules (Rules& rules);

    /**
     * @brief Returns the eight scout rules: the seven config rules and `toolchain`.
     *
     * `toolchain` fails a `## toolchain` table whose header row lacks a
     * `command` column or a `flag` column. The failure names the table and
     * the header row.
     *
     * @return The rules, keyed by rule name, built once.
     */
    const Rules& getRules() const override;
};
