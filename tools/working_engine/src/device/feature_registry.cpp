#include "feature_registry.hpp"

namespace casio::feature
{

const FeatureSpec* find_by_alias(const char* name)
{
    if (!name) return nullptr;
    for (int i = 0; i < FEATURE_COUNT; i++) {
        const FeatureSpec& f = FEATURES[i];
        for (int j = 0; j < f.alias_count; j++) {
            if (f.aliases[j] && name[0] == f.aliases[j][0]) {
                int matched = 0;
                while (name[matched] && f.aliases[j][matched]) {
                    if (name[matched] != f.aliases[j][matched]) break;
                    matched++;
                }
                if (name[matched] == '\0' && f.aliases[j][matched] == '\0') {
                    return &f;
                }
            }
        }
    }
    return nullptr;
}

const FeatureSpec* find_by_id(FeatureId id)
{
    for (int i = 0; i < FEATURE_COUNT; i++) {
        if (FEATURES[i].id == id) return &FEATURES[i];
    }
    return nullptr;
}

}