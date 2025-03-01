# MinimalDFA
Goal of this task is to implement algorithms for finding a minimal deterministic finite automaton that accepts intersection or union of two languages defined by two finite automata.
These functions will have the following C++ signatures:
 DFA unify(const NFA&, const NFA&); , and
 DFA intersect(const NFA&, const NFA&); .
 Both of these functions must not only find the intersection (or union) but the finite automaton must also be minimal.
 The input and output of these algorithms are finite automata in the form of structures NFA, and DFA, representing the non
deterministic finite automaton and deterministic finite automaton, respectively. Of course, some automata in the NFA structure
 can also be viewed as deterministic if the transition function can be trivially converted to the DFA's transition function. These
 structs are only simple data structures for representation of finite automata. They don't validate its content in any way; correct
 initialization and content (with respect to DFA/NFA definition) is responsibility of the person creating the structure.

  The inputs of the unify and intersect function is guaranteed to be valid nondeterministic finite automaton. Namely, the
 guaranteed properties are:
 The sets of states (NFA::m_States), and alphabet symbols (NFA::m_Alphabet) will be non-empty.
 Initial state (NFA:m_InitialState) and final states (NFA::m_FinalStates) will be elements of the set of states
 (NFA::m_States).
 If there is no transition for a combination of a state q and alphabet symbol a, the key (q, a) will not exist in the map of
 transitions (NFA::m_Transitions) at all.
 The transition function (NFA::m_Transitions) only uses states and symbol specified in the set of states and the
 alphabet.
