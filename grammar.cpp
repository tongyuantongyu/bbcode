//
// Created by TYTY on 2021-01-01 001.
//

#include <sstream>
#include <limits>
#include <mutex>
#include <cmath>
#include "grammar.h"

namespace bbcode::grammar {

std::vector<const char *> CONSTANTS = {
    ":)",
    ":(",
    ":-)",
    "{:1_01:}",
    "{:1_02:}",
    "{:1_03:}",
    "{:1_04:}",
    "{:1_05:}",
    "{:1_06:}",
};

static std::unordered_set<std::string> COLORS = {
    "black",
    "silver",
    "gray",
    "white",
    "maroon",
    "red",
    "purple",
    "fuchsia",
    "green",
    "lime",
    "olive",
    "yellow",
    "navy",
    "blue",
    "teal",
    "aqua",
    "orange",
    "aliceblue",
    "antiquewhite",
    "aquamarine",
    "azure",
    "beige",
    "bisque",
    "blanchedalmond",
    "blueviolet",
    "brown",
    "burlywood",
    "cadetblue",
    "chartreuse",
    "chocolate",
    "coral",
    "cornflowerblue",
    "cornsilk",
    "crimson",
    "darkblue",
    "darkcyan",
    "darkgoldenrod",
    "darkgray",
    "darkgreen",
    "darkgrey",
    "darkkhaki",
    "darkmagenta",
    "darkolivegreen",
    "darkorange",
    "darkorchid",
    "darkred",
    "darksalmon",
    "darkseagreen",
    "darkslateblue",
    "darkslategray",
    "darkslategrey",
    "darkturquoise",
    "darkviolet",
    "deeppink",
    "deepskyblue",
    "dimgray",
    "dimgrey",
    "dodgerblue",
    "firebrick",
    "floralwhite",
    "forestgreen",
    "gainsboro",
    "ghostwhite",
    "gold",
    "goldenrod",
    "greenyellow",
    "grey",
    "honeydew",
    "hotpink",
    "indianred",
    "indigo",
    "ivory",
    "khaki",
    "lavender",
    "lavenderblush",
    "lawngreen",
    "lemonchiffon",
    "lightblue",
    "lightcoral",
    "lightcyan",
    "lightgoldenrodyellow",
    "lightgray",
    "lightgreen",
    "lightgrey",
    "lightpink",
    "lightsalmon",
    "lightseagreen",
    "lightskyblue",
    "lightslategray",
    "lightslategrey",
    "lightsteelblue",
    "lightyellow",
    "limegreen",
    "linen",
    "mediumaquamarine",
    "mediumblue",
    "mediumorchid",
    "mediumpurple",
    "mediumseagreen",
    "mediumslateblue",
    "mediumspringgreen",
    "mediumturquoise",
    "mediumvioletred",
    "midnightblue",
    "mintcream",
    "mistyrose",
    "moccasin",
    "navajowhite",
    "oldlace",
    "olivedrab",
    "orangered",
    "orchid",
    "palegoldenrod",
    "palegreen",
    "paleturquoise",
    "palevioletred",
    "papayawhip",
    "peachpuff",
    "peru",
    "pink",
    "plum",
    "powderblue",
    "rosybrown",
    "royalblue",
    "saddlebrown",
    "salmon",
    "sandybrown",
    "seagreen",
    "seashell",
    "sienna",
    "skyblue",
    "slateblue",
    "slategray",
    "slategrey",
    "snow",
    "springgreen",
    "steelblue",
    "tan",
    "thistle",
    "tomato",
    "turquoise",
    "violet",
    "wheat",
    "whitesmoke",
    "yellowgreen",
    "rebeccapurple",
};

static std::unordered_set<std::string> ABS_SIZES = {
    "xx-small",
    "x-small",
    "small",
    "medium",
    "large",
    "x-large",
    "xx-large"
};

static std::unordered_set<std::string> SIZE_UNITS = {
    "em",
    "px",
    "rem"
};

ValidateResult verifyColor(std::string_view s, const MessageEmitter &warn) {
  std::string lower;
  std::transform(s.begin(),
                 s.end(),
                 std::back_inserter(lower),
                 [](const i8 &c) {
                   return std::tolower(c);
                 });

  std::stringstream ss;

  if (COLORS.contains(lower)) {
    if (s != lower) {
      ss << "Color keyword contains upper character: `" << s << "`";

      warn(Message{
          .severity = Tidy,
          .offset = 0,
          .span = s.size(),
          .name = "color-upper-keyword",
          .message = ss.str(),
      });
    }

    return ValidateResult{
        .result = ValidateResult::Ok,
        .content = std::move(lower)
    };
  }

  if ((lower.size() == 4 || lower.size() == 7 || lower.size() == 9) &&
      lower[0] == '#' &&
      std::all_of(++lower.begin(), lower.end(), [](const i8 &c) {
        return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f');
      })) {
    if (s != lower) {
      ss << "Hex color contains upper character: `" << s << "`";

      warn(Message{
          .severity = Tidy,
          .offset = 0,
          .span = s.size(),
          .name = "color-upper-hex",
          .message = ss.str(),
      });
    }

    return ValidateResult{
        .result = ValidateResult::Ok,
        .content = std::move(lower)
    };
  }

  ss << "Invalid color parameter: can't recognize `" << s
     << "` as color keyword or hex color.";
  return ValidateResult{
      .result = ValidateResult::Error,
      .content = ss.str()
  };
}

void trim(std::string_view &sv, const std::unordered_set<i8> &c) {
  while (!sv.empty() && c.contains(sv.front())) {
    sv.remove_prefix(1);
  }

  while (!sv.empty() && c.contains(sv.back())) {
    sv.remove_suffix(1);
  }
}

std::unordered_multimap<std::string, NodeDescriptor> NODE_MAP = {
    // [b][/b] bold text
    {"b", {
        .type = Simple,
        .name = "b",
        .level = 1,
    }},

    {"x", {
      .type = Simple,
      .name = "x",
      .level = 1,
    }},

    // [i][/i] italic text
    {"i", {
        .type = Simple,
        .name = "i",
        .level = 1,
    }},

    // [center][/center] center text
    {"center", {
        .type = Simple,
        .name = "center",
        .level = 0,
    }},

    // [hr] split line
    {"hr", {
        .type = Omission,
        .name = "hr",
        .level = 0,
    }},

    // [code][/code] monospace text
    {"code", {
        .type = Verbatim,
        .name = "code",
        .level = 1,
    }},

    // [font=serif][/font] text with font
    {"font", {
        .type = Parametric,
        .name = "font",
        .level = 1,
        .d = NodeParametricDescriptor{
            .validator = [](std::string_view s, const MessageEmitter &warn) {
              usize start = 0;
              usize end = s.find(',');
              std::vector<std::string_view> fonts;
              std::stringstream ss;

              do {
                auto font = s.substr(start, end - start);
                auto org_font = font;
                trim(font, {' ', '"', '\''});

                if (!font.empty()) {
                  if (org_font != font) {
                    ss << "Font name contains space or quote: `" << org_font
                       << "`";

                    warn(Message{
                        .severity = Tidy,
                        .offset = start,
                        .span = org_font.size(),
                        .name = "font-name-dirty",
                        .message = ss.str()
                    });

                    ss.str("");
                  }

                  fonts.emplace_back(font);
                }
                else {
                  warn(Message{
                      .severity = Tidy,
                      .offset = start,
                      .span = org_font.size(),
                      .name = "font-name-empty",
                      .message = "Empty font name ignored"
                  });
                }

                start = end + 1;
                end = s.find(',', start);
              } while (end != std::string_view::npos);

              for (const auto &font : fonts) {
                ss << font << ',';
              }

              std::string result = ss.str();
              result.erase(--result.end());

              return ValidateResult{
                  .result = ValidateResult::Ok,
                  .content = std::move(result)
              };
            }
        }
    }},

    // [size=7][/size] text with size
    {"size", {
        .type = Parametric,
        .name = "size",
        .level = 1,
        .d = NodeParametricDescriptor{
            .validator = [](std::string_view s, const MessageEmitter &warn) {
              std::stringstream ss;
              std::istringstream is((std::string(s)));
              f64 size = 0;
              std::string remain;

              is >> size;
              if (is.fail()) {
                size = std::numeric_limits<f64>::quiet_NaN();
                is.clear();
              }

              is >> remain;

              if (remain.empty()
                  && !std::isnan(size)) {
                i32 isize = i32(size);
                if (std::abs(size - f64(isize))
                    < std::numeric_limits<f64>::epsilon()) {
                  if (isize >= 1 && isize <= 7) {
                    return ValidateResult{
                        .result = ValidateResult::Ok,
                        .content = std::to_string(isize)
                    };
                  }
                }

                ss << "Invalid numeric absolute size: `" << s << "`";
                return ValidateResult{
                    .result = ValidateResult::Error,
                    .content = ss.str()
                };
              }
              else if (!std::isnan(size)) {
                std::string lower;
                std::transform(remain.begin(),
                               remain.end(),
                               std::back_inserter(lower),
                               [](const i8 &c) {
                                 return std::tolower(c);
                               });

                if (ABS_SIZES.contains(lower)) {
                  if (s != lower) {
                    ss << "Absolute size keyword contains upper character: `"
                       << s << "`";

                    warn(Message{
                        .severity = Tidy,
                        .offset = 0,
                        .span = s.size(),
                        .name = "size-upper-keyword",
                        .message = ss.str(),
                    });

                    ss.str("");
                  }

                  return ValidateResult{
                      .result = ValidateResult::Ok,
                      .content = std::move(lower)
                  };
                }
                else {
                  ss << "Invalid size parameter: can't recognize `" << s
                     << "` as keyword or united size.";
                  return ValidateResult{
                      .result = ValidateResult::Error,
                      .content = ss.str()
                  };
                }
              }
              else {
                if (size < 0) {
                  ss
                      << "Invalid size parameter: expect non-negative number, found: `"
                      << size << "`";
                  return ValidateResult{
                      .result = ValidateResult::Error,
                      .content = ss.str()
                  };
                }

                std::string lower;
                std::transform(remain.begin(),
                               remain.end(),
                               std::back_inserter(lower),
                               [](const i8 &c) {
                                 return std::tolower(c);
                               });

                if (SIZE_UNITS.contains(lower)) {
                  if (s != lower) {
                    ss << "size unit contains upper character: `" << s << "`";

                    warn(Message{
                        .severity = Tidy,
                        .offset = 0,
                        .span = s.size(),
                        .name = "size-upper-unit",
                        .message = ss.str(),
                    });

                    ss.str("");
                  }

                  ss << size << lower;

                  return ValidateResult{
                      .result = ValidateResult::Ok,
                      .content = ss.str()
                  };
                }
                else {
                  ss << "Invalid size parameter: unknown size unit `" << remain
                     << "`";
                  return ValidateResult{
                      .result = ValidateResult::Error,
                      .content = ss.str()
                  };
                }
              }
            }
        }
    }},

    // [color=#aaa][/color] text with color
    {"color", {
        .type = Parametric,
        .name = "color",
        .level = 1,
        .d = NodeParametricDescriptor{
            .validator = verifyColor
        }
    }},

    // [url=http://xxx.xx.xxx][/url] link
    {"url", {
        .type = Parametric,
        .name = "url",
        .level = 1,
        .d = NodeParametricDescriptor{
            .validator = [](std::string_view s, const auto &) {
              return ValidateResult{
                  .result = ValidateResult::Ok,
                  .content = std::string(s)
              };
            }
        }
    }},

    // [list][/list] unordered list
    {"list", {
        .type = Simple,
        .name = "list",
        .level = 0,
        .children = {"*"}
    }},

    // [list=1][/list] ordered list
    {"list", {
        .type = Parametric,
        .name = "list",
        .level = 0,
        .d = NodeParametricDescriptor{
            .validator = [](std::string_view s, const MessageEmitter &) {
              if (s == "a" || s == "1") {
                return ValidateResult{
                    .result = ValidateResult::Ok,
                    .content = std::string(s)
                };
              }
              else {
                std::stringstream ss;
                ss
                    << "Invalid ordered list parameter: unknown index indicator `"
                    << s << "`";

                return ValidateResult{
                    .result = ValidateResult::Error,
                    .content = ss.str()
                };
              }
            }
        },
        .children = {"*"}
    }},

    // [*] list item
    {"*", {
        .type = Greedy,
        .name = "*",
        .level = 0,
        .d = NodeGreedyDescriptor{
            .terminator = {"list"}
        }
    }},

    // [table][/table] table
    {"table", {
        .type = Simple,
        .name = "table",
        .level = 0,
        .children = {"tr"}
    }},

    // [table=black][/table] table with background color
    {"table", {
        .type = Parametric,
        .name = "table",
        .level = 0,
        .d = NodeParametricDescriptor{
            .validator = verifyColor
        },
        .children = {"tr"}
    }},

    // [tr][/tr] table row
    {"tr", {
        .type = Simple,
        .name = "tr",
        .level = 0,
        .children = {"td"},
        .parents = {"table"}
    }},

    // [td][/td] table cell
    {"td", {
        .type = Simple,
        .name = "td",
        .level = 0,
        .parents = {"tr"}
    }},
};
decltype(NODE_MAP)::iterator get_descriptor(const std::string &name,
                                            NodeType type) {
  auto match = NODE_MAP.equal_range(name);
  auto descriptor = match.first;
  while (descriptor != match.second && descriptor->second.type != type) {
    ++descriptor;
  }

  if (descriptor == match.second) {
    return NODE_MAP.end();
  } else {
    return descriptor;
  }
}

}
