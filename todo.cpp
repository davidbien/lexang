// 1) Don't partition the dead state immediately upon entering the optimizer.
//     This should allow regions of the graph that are equivalent to the dead
//     state to be pruned entirely - this could make the pattern exclusion code
//     create non-hungry DFAs. Must also not assume dead state is a singleton
//     anymore, etc.
// 2) create utility interfaces that allow easy generation of 