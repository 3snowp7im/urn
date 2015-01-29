# About

This simple split tracker was hacked together by 3snow p7im.
It was originally written because there were no exisiting
solutions for split tracking with a delayed start available
on *nix platforms.

# Usage

Without a game loaded, the timer looks like an empty,
black box. You can load a game from the menu, by pressing
Escape. Once a game is loaded, use the spacebar and back-
space keys to control the timer.

| Key       | Stopped     | Started    |
|-----------|-------------|------------|
| Spacebar  | Start timer | Split time |
| Backspace | Reset timer | Stop timer |

The color of a time or delta has special meaning.

| Color | Meaning                      |
|-------|------------------------------|
| Red   | Behind splits in PB          |
| Green | Ahead of splits in PB        |
| Blue  | Best split time in any run   |
| Gold  | Best segment time in any run |
| Gray  | Split time for current run   |
