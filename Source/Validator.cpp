#include "Validator.h"
#include "Sampler.h"

void Validator::addRules (Rules& rules)
{
    jam::ConfigValidator::addRules (rules);

    rules.add<const jam::Document&> (Id::toolchain.toString(),
        [] (const jam::Document& document) -> juce::Result
        {
            const auto& config { static_cast<const jam::ConfigDocument&> (document) };

            for (const auto* table : config.getTables (Id::toolchain))
            {
                const auto* headerRow { config.getTableRow (*table, Id::headerRow) };

                if (config.getTableCell (*headerRow, Id::command) == nullptr
                    or config.getTableCell (*headerRow, Id::flag) == nullptr)
                    return juce::Result::fail (getLocation (*table, *headerRow, Id::toolchain.toString())
                                               + Id::diagnosticSeparator
                                               + text::Diagnostics::failToolchainColumn);
            }

            return juce::Result::ok();
        });

    rules.add<const jam::Document&> (Id::sample.toString(),
        [] (const jam::Document& document) -> juce::Result
        {
            const auto& config { static_cast<const jam::ConfigDocument&> (document) };

            for (const auto* table : config.getTables (Id::sample))
            {
                const auto intervalMs { config.getTableValue (*table, Id::value, Id::intervalMs).getIntValue() };
                const auto stackDepth { config.getTableValue (*table, Id::value, Id::stackDepth).getIntValue() };

                if (intervalMs < Sampler::minimumIntervalMs
                    or stackDepth < Sampler::minimumStackDepth
                    or stackDepth > Sampler::maximumStackDepth)
                    return juce::Result::fail (getLocation (*table, *config.getTableRow (*table, Id::headerRow), Id::sample.toString())
                                               + Id::diagnosticSeparator
                                               + text::Diagnostics::failSampleRange);
            }

            return juce::Result::ok();
        });
}

const Validator::Rules& Validator::getRules() const
{
    static const auto rules {
        []
        {
            Rules rules;
            addRules (rules);
            return rules;
        }()
    };

    return rules;
}
