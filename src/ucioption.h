// Superpawn, a UCI chess engine.
// Copyright (C) 2026 John Byrd and the Superpawn contributors.
// Licensed under the GNU Affero General Public License v3.0 or later.

#pragma once

#include <functional>
#include <iosfwd>
#include <map>
#include <string>

namespace sp {

class Option;

// Case-insensitive comparison, since UCI option names are case-insensitive.
struct CaseInsensitiveLess {
    bool operator()(const std::string&, const std::string&) const;
};

// All UCI options, in the order they were added, printed in reply to `uci`.
class OptionsMap {
   public:
    void          add(const std::string& name, const Option& option);
    void          set(const std::string& name, const std::string& value);
    const Option& operator[](const std::string& name) const;
    bool          contains(const std::string& name) const { return options.count(name) > 0; }

    friend std::ostream& operator<<(std::ostream&, const OptionsMap&);

   private:
    std::map<std::string, Option, CaseInsensitiveLess> options;
};

// A single option: spin, check, string, combo or button. The on_change callback
// runs whenever the value is set, including at startup with the default.
class Option {
   public:
    using OnChange = std::function<void(const Option&)>;

    Option(OnChange f = nullptr);                                  // button
    Option(bool v, OnChange f = nullptr);                          // check
    Option(const char* v, OnChange f = nullptr);                   // string
    Option(int v, int minv, int maxv, OnChange f = nullptr);       // spin
    Option(const char* v, const char* cur, OnChange f = nullptr);  // combo

    Option& operator=(const std::string& v);

    operator int() const;
    operator std::string() const;

   private:
    friend class OptionsMap;
    friend std::ostream& operator<<(std::ostream&, const OptionsMap&);

    std::string defaultValue, currentValue, type;
    int         min = 0, max = 0;
    std::size_t idx = 0;
    OnChange    on_change;
};

}  // namespace sp
