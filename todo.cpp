// 1) Don't partition the dead state immediately upon entering the optimizer.
//     This should allow regions of the graph that are equivalent to the dead
//     state to be pruned entirely - this could make the pattern exclusion code
//     create non-hungry DFAs. Must also not assume dead state is a singleton
//     anymore, etc.
// 2) create utility interfaces that allow easy generation of 


// Triggers, as currently implemented, don't work correctly.
// They could however, I believe, live up to their unforementioned hype.
// I believe that there are a few reasons they are failing:
// 1) And this isn't catastrophic at least currently, the triggers use the same
//  pseudo alphabet indicator (AlphabetSize). Each trigger should use a unique alphabet symbol.
//  This will ensure that all triggers fire and as well will point out invalid trigger configurations, etc.
// 2) I believe that the dead state transitions that are added for optimization are causing invalid
//  optimized DFAs to be created. The solution is, when adding the dead states, we check to see if a node
//  has any trigger transtions out of it. If there is a single trigger out of it, or multiple triggers that to *go to the same state*
//  then instead of adding all non-represented transitions as transtions to the dead state, we add those transtions
//  as transtions to the trigger state (as this in reality is what would happen while parsing).
//    If there are no triggers then we create the transitions to the dead state as normal.
// 3) If two (different) triggers leave a state (they would have to be different) and they don't go to the same
//  state then we have a "trigger ambiguity" - it is unclear which trigger should fire. This will be discovered
//  programmatically and we will abort generation of the optimized DFA.
// 4) So in (2) above we then will have a trigger fire on a nonconsecutive set of inputs. These somehow have to be marked as
//  trigger transitions so that they are be appropriately processed when parsing. In fact they can be pruned from the final optimized
//  graph we we will understand that 


// 1) Rename the actions according to the token that is activated by the action. Also rename the action object. This is less confusing.
// 2) Make the action object members of the LexAnal - not static members.
// 3) Code up some stream objects for general use with in the LexAnal.
// 4) Output any actions/triggers that are not referenced in the resultant DFA so that this can be used to debug any problems with the reg exps.
// 5) Data associated with a given token needs to be packetized and stored with the token on a stack that the parser thread can then pop from.
//    My guess is that the parser thread would be waiting a lot since most of the work would be done by the lexicographical analyzer.
//    So this may not increase the speed of parsing and if no perhaps not by much.

// 1) Add regexp_not_set and regexp_not_range to add "everything but what is in this set" functionality.
// 2) Add the ability to have utility action objects. These don't have a trigger point but they can be used for data storage by other objects. They must have unique token ids.
//    They should be in the trigger range - i.e. they have a bit in the trigger bits. Tokens are not allocated bits in the trigger bits since only one can be returned at any given time.
