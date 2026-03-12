# ouo

A little statically-typed interpreted language I'm making to learn more about C,
languages, and myself.

## Status

- Has ints and floats that you can add and multiply together. With type
  checking!
- Has a working LSP server that reports static analysis diagnostics.

### Milestones

- [ ] Make it good enough to solve Advent of Code.
- [ ] Implement everything from the [draft](DRAFT.md).

## Attributions

Some amazing resources that were very helpful to me:

- Thorsten Ball's [Writing An Interpreter In Go](https://interpreterbook.com/) -
  the first book about language development that I read a long time ago, the
  main thing that inspired me to create a language.
- Daniel Holden's [Build Your Own Lisp](https://buildyourownlisp.com/) - another
  very good book. Very short and easy, picked this one specifically to get more
  familiar with C.
- Robert Nystrom's [Crafting Interpreters](https://craftinginterpreters.com/) -
  I initially ignored this one because I thought it will be using Java
  throughout its entirety, but when I learned that the second part
  reimplements everything in C I binged the whole thing. Served as a good basis
  for the current language implementation.
- Tsoding's [nob.h](https://github.com/tsoding/nob.h/blob/main/nob.h) - just
  reading the source code of this library tought me a lot of (hopefully) good C
  practices. I use its helper macros extensively.
- Tsoding's [JIM](https://github.com/tsoding/jim) - a very cool way to parse
  and serialize JSON without and intermediate representation that I use in the
  current LSP implementation.
