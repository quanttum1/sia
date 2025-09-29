# sia

**sia** is a simple & cute terminal-based (speed)-reader for plain text files that loves and respects you as a user ❤️
It displays your text word by word at certain speed, with simple interactive controls.
When you quit, it removes everything you’ve already read from the file so you can continue where you left off next time.

⚠️ Windows support is incomplete right now (POSIX/Linux/macOS works fine).

---

## Features

* Reads plain text files word by word, at a customisable speed (characters per minute).
* Non-blocking keyboard input:

  * **Space** → pause/resume
  * **q** → quit and save progress
  * **-** → slow down (−200 cpm)
  * **+ or =** → speed up (+200 cpm)
* Progress persistence: when you quit with `q`, the already-read part of the file is deleted.
* UTF-8 aware (basic multi-byte character handling).
* Cute, minimal, self-contained single C file.

---

## Installation

Clone the project and build it with `gcc`:

```bash
gcc main.c -o sia
mv sia ~/.local/bin
```

Make sure `~/.local/bin` is in your `$PATH`.

## Usage

```bash
sia <file.txt>
```

---
## Contributing/Support
Any contributions are very welcome! Feel free to open an issue or a pull request :3
There are a lot of TODOs I didn't implement yet, so there's always something to do 😅

## License

Unlicense (Public Domain)
---

Happy reading! <3
