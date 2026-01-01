# libbgt(Basic Graphic Tools)

A lightweight, cross-platform graphics library for C++ education, built on SDL3.

## Purpose

libbgt is designed specifically for teaching C++ programming. It is purposed to provide multiple API sets that progress from simple C-style functions to full object-oriented designs, adapting to different learning stages. By hiding complex concepts like double-buffering and event loops, students who have just learned basic syntax can immediately create graphical applications and games, experiencing "what you code is what you see".

## Key Features

- Multiple API Styles(In Progress):
  - [x] Level 1: Pure C-style functions (no structs/classes)
  - [ ] Level 2: C with simple structs(Under development)
  - [ ] Level 3: Complete C++ object-oriented interface(Under development)
- Beginner-Friendly: Start with simple functions, progress gradually.
- Zero-Configuration: Vendored static library distribution means just one header and library file needed.
- Cross-Platform: Runs on Windows, MacOS, and Linux.
- Immediate Visual Feedback: Drawing functions provide instant results.
- Simple Input Handling: Easy keyboard and mouse state checking.
- Educational Focus: Clear source code that can be studied as a teaching resource.
- Multiple Encoding Support: Students can use local encodings such as GB2312.

## Development

### Prerequisites

- C++23 or later compiler
- [XMake](https://xmake.io)

### Build Steps

```bash
git clone https://github.com/Tongji-High-level-Language-Programming/libbgt.git

# Configure project
xmake config --use-ansi=[y|n]

# build main library and tests
xmake

# optional: build vendored static library
xmake libbgt_vendored
```

## License

MIT License - see LICENSE file for details.
