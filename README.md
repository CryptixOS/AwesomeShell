# AwesomeShell

Version: 0.2.0

AwesomeShell is a lightweight, yet powerful Unix-like shell designed for advanced scripting and command execution. It combines a robust lexer with a sophisticated AST engine, enabling precise parsing and execution of complex shell commands.

## Features
- Command Execution: Run system commands with full support for chaining (&&, ||) and sequences (;).
- Variables & Assignment: Define and use shell variables dynamically. Supports $? to access last command exit status.
- Advanced Lexer & AST: Parses input into a detailed abstract syntax tree, covering most shell node types for accurate evaluation.


## How It Works
AwesomeShell processes input in three stages:

1. Lexing
The lexer tokenizes input into structured tokens
 
2. AST Construction
After lexing, the parser builds an advanced abstract syntax tree (AST) representing the hierarchical structure of commands:
```
SequenceNode
├── CommandNode: VAR=hello
├── BackgroundNode
│   └── SubshellNode
│       └── LogicalNode (OR)
│           ├── LogicalNode (AND)
│           │   ├── PipelineNode
│           │   │   ├── CommandNode: echo $VAR
│           │   │   │   └── VariableNode: $VAR
│           │   │   └── CommandNode: grep h
│           │   │       └── LiteralNode: h
│           │   └── CommandNode: cat < out.txt
│           │       └── RedirectionNode: < out.txt
│           └── CommandNode: echo "fail"
│               └── LiteralNode: "fail"
├── CommandNode: echo $(date) >> log.txt
│   ├── CommandSubstNode: $(date)
│   │   └── CommandNode: date
│   └── RedirectionNode: >> log.txt
```

3. Lowering & Execution
The AST is then lowered into intermediate code by the lowerer. This intermediate representation is finally passed to the executor, which performs the actual command execution while respecting variables, logical operators.

## Examples

### Variable assignment

VAR=hello

### Command chaining

echo $VAR && echo "World" || echo "Failed"


## License

AwesomeShell is licensed under the GPL-3.0. See LICENSE for details.
