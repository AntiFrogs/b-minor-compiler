# Syntax highlighting

## From tokens and AST to source ranges

Both `--ansi` and `--html` parse the program with recovery. The highlighter then scans the source to obtain the token ranges, including comments. It assigns a baseline style to each token and uses source offsets to add two kinds of context:

- A function declaration AST node marks the name in a prototype or definition. A call AST node marks its callee when the callee is an identifier. This also handles a grouped callee such as `(repeat)(2)` because the identifier token retains its exact position.
- A diagnostic marks the token at the reported position, overriding its usual style. Invalid lexer tokens are marked even if they fall inside a discarded construct. EOF has zero width, so a diagnostic there appears on standard error only.

The classification applies to individual occurrences. A variable can share a spelling with a function and still use the default style. Calls are classified from syntax. Name binding and checking whether a callee is callable are later semantic work. Tokens inside a discarded construct still receive baseline styles.

## Terminal styles

| Source category | Style |
|---|---|
| Keywords such as `if`, `return`, `array`, and `function` | Bold blue |
| Built-in type names, including `void` | Cyan |
| Variable and parameter identifiers | Terminal default |
| Function names and direct call sites | Gold |
| Decimal integers and boolean literals | Orange |
| String and character literals | Green |
| Operators | Light gray |
| Line and block comments | Dim, italic gray |
| Invalid tokens and tokens at syntax diagnostics | Red underline |
| Other delimiters and whitespace | Terminal default |

The renderer walks the ordered ranges over the original source and writes the intervening bytes as well. It resets the ANSI style after each styled range. The source text retains its indentation, tabs, line endings, and final newline state. ASCII control bytes other than tab, CR, and LF, plus DEL, display as `\xHH`. The exact colors depend on the terminal palette and color support.

## Standalone HTML

The HTML renderer walks the same ranges and uses spans with classes such as `kw`, `fn`, `lit`, and `error`. The page contains its stylesheet and places source in `<pre><code>`. The code starts immediately after `<code>` and ends immediately before `</code>`, preserving leading and trailing newlines from the input. Tabs remain in the text and display at four-column stops. The page uses horizontal scrolling for long lines.

Source characters with HTML syntax are written as entities in both styled and unstyled text. Carriage returns use a character reference to retain their code points in the code block. Valid UTF-8 characters within a source range are copied. Other nonprintable ASCII bytes and bytes that cannot form a complete UTF-8 character within their range display as `\xHH`. Diagnostics go to standard error, so redirecting standard output yields just the HTML document.
