// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#include "ucioption.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <ostream>
#include <sstream>
#include <string>

namespace sp {

bool CaseInsensitiveLess::operator()(const std::string& s1, const std::string& s2) const {
    return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(),
                                        [](char c1, char c2) { return std::tolower(c1) < std::tolower(c2); });
}

Option::Option(OnChange f) : type("button"), on_change(std::move(f)) {}

Option::Option(bool v, OnChange f) : type("check"), on_change(std::move(f)) {
    defaultValue = currentValue = (v ? "true" : "false");
}

Option::Option(const char* v, OnChange f) : type("string"), on_change(std::move(f)) {
    defaultValue = currentValue = v;
}

Option::Option(int v, int minv, int maxv, OnChange f) : type("spin"), min(minv), max(maxv), on_change(std::move(f)) {
    defaultValue = currentValue = std::to_string(v);
}

Option::Option(const char* v, const char* cur, OnChange f) : type("combo"), on_change(std::move(f)) {
    defaultValue = v;
    currentValue = cur;
}

Option::operator int() const {
    assert(type == "check" || type == "spin");
    return type == "spin" ? std::stoi(currentValue) : currentValue == "true";
}

Option::operator std::string() const {
    assert(type == "string" || type == "combo");
    return currentValue;
}

// Validates and stores a new value, then runs the callback. Out-of-range spins
// and malformed values are ignored.
Option& Option::operator=(const std::string& v) {
    assert(!type.empty());

    if (type == "button") {
        if (on_change)
            on_change(*this);
        return *this;
    }

    if (type == "check" && v != "true" && v != "false")
        return *this;

    if (type == "spin") {
        int value;
        try {
            value = std::stoi(v);
        } catch (...) {
            return *this;
        }
        if (value < min || value > max)
            return *this;
        currentValue = std::to_string(value);
    } else if (type == "combo") {
        // The combo's default string lists the alternatives, e.g. "a var a var b"
        std::istringstream ss(defaultValue);
        std::string        token;
        bool               found = false;
        while (ss >> token)
            if (token != "var" && token == v)
                found = true;
        if (!found)
            return *this;
        currentValue = v;
    } else
        currentValue = v;

    if (on_change)
        on_change(*this);

    return *this;
}

void OptionsMap::add(const std::string& name, const Option& option) {
    auto& o = options[name];
    o       = option;
    o.idx   = options.size() - 1;

    // Run the callback with the default so that dependents are initialised
    if (o.on_change && o.type != "button")
        o.on_change(o);
}

void OptionsMap::set(const std::string& name, const std::string& value) {
    auto it = options.find(name);
    if (it != options.end())
        it->second = value;
}

const Option& OptionsMap::operator[](const std::string& name) const {
    auto it = options.find(name);
    assert(it != options.end());
    return it->second;
}

std::ostream& operator<<(std::ostream& os, const OptionsMap& om) {
    for (std::size_t idx = 0; idx < om.options.size(); ++idx)
        for (const auto& [name, o] : om.options)
            if (o.idx == idx) {
                os << "\noption name " << name << " type " << o.type;

                if (o.type == "check" || o.type == "combo")
                    os << " default " << o.defaultValue;
                else if (o.type == "string")
                    os << " default " << (o.defaultValue.empty() ? "<empty>" : o.defaultValue);
                else if (o.type == "spin")
                    os << " default " << o.defaultValue << " min " << o.min << " max " << o.max;

                break;
            }

    return os;
}

}  // namespace sp
