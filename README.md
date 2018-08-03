# wowl-chess
Wowl is a basic chess engine written in C++.<br />
Requires SFML to compile.

## Board representation
* 10x12 mailbox

## Search
* Iterative deepening with aspiration window
* Transposition table
* Quiescence search
* SEE
* Killer moves
* Null move pruning
* PVS
* Late move reduction

## Evaluation
* Material evaluation with piece-square tables
* King safety
* Space and center control
* Pawn structure
* Passed pawns