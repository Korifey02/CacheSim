# CacheSim Development Roadmap

## Phase 0 — Safety & Testability Foundation
+ 0.1 Create test .c files with reference cache simulation results
+ 0.2 Replace longjmp with C++ exceptions (throw/catch)
- 0.3 Fix memory leaks (Sim destructor, array dealloc on scope exit)
- 0.4 Add bounds checking for fixed-size arrays

## Phase 1 — Eliminate Parser Duplication
- 1.1 Introduce EvalMode abstraction (compute values vs trace accesses)
- 1.2 Merge parser.cpp + parser_sim.cpp into single parser with mode
- 1.3 Delete parser_sim.cpp
- 1.4 Remove SIMULATOR macro, make FAST_SIMULATOR runtime choice

## Phase 2 — Lexer: Token Stream
- 2.1 Create Token struct {type, value, line, source_pos}
- 2.2 Write tokenize() — one-pass tokenization of entire file
- 2.3 Switch parser to Token* navigation instead of G_PROGRAM_POINTER
- 2.4 Replace get_token()/putback() with next_token()/peek_token()
- 2.5 Refactor FAST_SIMULATOR plan to use token indices

## Phase 3 — Language Extension: Operators
- 3.1 ++ and -- (post/prefix)
- 3.2 +=, -=, *=, /=, %=
- 3.3 &&, ||, !
- 3.4 &, |, ^, ~, <<, >>
- 3.5 Ternary ? :
- 3.6 Comma operator

## Phase 4 — Extended Type System
- 4.1 eval_exp with tagged union Value instead of int*
- 4.2 float/double variables (not just arrays)
- 4.3 Init at declaration: int x = 5;
- 4.4 Arrays as function parameters
- 4.5 Multiple declarations: int x, y, z; (verify)

## Phase 5 — AST (optional, based on Phase 2 benchmarks)
- 5.1 Define AST nodes
- 5.2 Parser builds AST instead of immediate execution
- 5.3 Interpreter::execute(ASTNode*)
- 5.4 Integrate cache trace into AST walk

## Phase 6 — Additional Language Constructs
- 6.1 switch/case/default
- 6.2 Multi-dimensional arrays int a[N][M]
- 6.3 #define (simple constants)
- 6.4 sizeof
- 6.5 Built-in functions: sqrt, abs, pow, printf

## Phase 7 — Large Data Optimization
- 7.1 unordered_map for variable/array lookup
- 7.2 Dynamic containers instead of fixed arrays
- 7.3 Remove PROG_SIZE limit
- 7.4 Fix nested loops in FAST_SIMULATOR (first_iter is global scalar)
- 7.5 Profiling and hot spot elimination

## Execution Order
Phases 0→1→2 strictly sequential.
Phases 3,4,6,7 can partially overlap.
Phase 5 decided after Phase 2 benchmarks.

## Current Status
- All phases: NOT STARTED
