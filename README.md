# BBCode Parser

Parse bbcode into node tree.

Input example:

```
BBCode example
[b][i]bold italic text[/i][/b]
```

Output example (original output is minified):

```json
[
  {
    "type": "Literal",
    "line": 0,
    "chr": 0,
    "offset": 0,
    "span": 14,
    "content": "BBCode example"
  },
  {
    "type": "Newline",
    "line": 0,
    "chr": 14,
    "offset": 14,
    "span": 1
  },
  {
    "type": "Simple",
    "line": 1,
    "chr": 0,
    "offset": 15,
    "span": 30,
    "name": "b",
    "content": [
      {
        "type": "Simple",
        "line": 1,
        "chr": 3,
        "offset": 18,
        "span": 23,
        "name": "i",
        "content": [
          {
            "type": "Literal",
            "line": 1,
            "chr": 6,
            "offset": 21,
            "span": 16,
            "content": "bold italic text"
          }
        ]
      }
    ]
  }
]
```

## Compile

This project requires CMake and a C++20 compiler.

```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

## Usage

`parser_tool` is the main CLI tool to do parse:

```sh
parser_tool [input_file] [output_file]
```

If a parameter is omitted or `-`, it will use standard input / output.

For using the parser as a library, please check source code of `parser_tool` for now.