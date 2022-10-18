
<p align="center">
    <img src="https://raw.githubusercontent.com/leflores-fisi/urn/master/static/urn.png" width=100 height=100/>
</p>
<h1 align="center">— Urn split tracker —</h1>

<p align="center">
    <img src="https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white"/>
    <img src="https://img.shields.io/github/stars/leflores-fisi/Urn.svg?style=for-the-badge&labelColor=black&logo=Github"/>
    <img src="https://img.shields.io/static/v1?label=Made%20with&message=GTK%203.0&color=725d9c&style=for-the-badge&logo=GTK&logoColor=white&labelColor=black"/>
    <img src="https://img.shields.io/github/license/leflores-fisi/Urn?label=license&style=for-the-badge&logo=GNU&logoColor=white&labelColor=black&color=b85353"/>
</p>

## About

> This simple split tracker was hacked together by [3snow p7im](https://github.com/3snowp7im).
> It was originally written because there were no existing
> solutions for split tracking with a delayed start available
> on *nix platforms.

**Forked** from the [original project](https://github.com/3snowp7im/urn) (rest in peace).

### Features added:
- Now the last split folder is saved ([pull request by @Thue](https://github.com/3snowp7im/urn/pull/49))
- New default theme (LiveSplit theme)
- New shortcut to keep window always on top (Ctrl+K)
- More friendly README.md
- Nicer time format
- New Urn icon ✨

### Bugs fixed:
- Timer kept running in the background while paused
- User was able to skip the last split

<p align="center">
    <img src="https://raw.githubusercontent.com/leflores-fisi/urn/master/static/screenshot.png"/>
</p>

---

## Quick start and installation

Urn requires `libgtk+-3.0`, `x11`, `libjansson`
and installing requires `imagemagick`, and `rsync`.

On Debian-based systems:
```sh
sudo apt update
sudo apt install libgtk-3-dev build-essential libjansson-dev imagemagick rsync
```

Clone the project:
```sh
git clone https://github.com/leflores-fisi/urn
cd urn
```

Now compile and install **Urn** (see details in the [Makefile](https://github.com/leflores-fisi/urn/blob/master/Makefile))
```sh
make
sudo make install
```

All ready! Now start the desktop **Urn** application or run `/usr/local/bin/urn-gtk`.

---

## ✨ Usage

When you start the **Urn** application, a file dialog will appear to select
a Split JSON file (see [Split files](#split-files)).

Initially, the window is undecorated. You can toggle window decorations
by pressing the `Right Control` key.


The timer is controlled by key presses:

| Key        | Stopped | Started |
|------------|---------|---------|
| <kbd>Spacebar</kbd>   | Start   | Split   |
| <kbd>Backspace</kbd>  | Reset   | Stop    |
| <kbd>Delete</kbd>     | Cancel  | -       |

Cancel will **reset the timer** and **decrement the attempt counter**. A run that is reset before the [start delay](#main-object) is automatically
canceled.

If you forgot to split, or accidentally split twice,
you can manually change the current split:

| Key       | Action      |
|-----------|-------------|
| <kbd>Page Up</kbd>   | Unsplit     |
| <kbd>Page Down</kbd> | Skip split  |

Keybinds can be configured by changing your gsettings.

## Settings and keybinds

See the [urn-gtk.gschema.xml](https://github.com/leflores-fisi/urn/blob/master/urn-gtk.gschema.xml) file.

| Setting                      | Type    | Description                        | Default |
|------------------------------|---------|------------------------------------|---------|
| `start-decorated`            | Boolean | Start with window decorations      | false   |
| `start-on-top`               | Boolean | Start with window as always on top | true    |
| `hide-cursor`                | Boolean | Hide cursor in window              | false   |
| `global-hotkeys`             | Boolean | Enables global hotkeys             | false   |
| `theme`                      | String  | Default theme name                 | ''      |
| `theme-variant`              | String  | Default theme variant              | ''      |

| Keybind                      |         |                                    | Default |
|------------------------------|---------|------------------------------------|---------|
| `keybind-start-split`        | String  | Start/split keybind                | <kbd>Space</kbd>
| `keybind-stop-reset`         | String  | Stop/Reset keybind                 | <kbd>Backspace</kbd>
| `keybind-cancel`             | String  | Cancel keybind                     | <kbd>Delete</kbd>
| `keybind-unsplit`            | String  | Unsplit keybind                    | <kbd>Page Up</kbd>
| `keybind-skip-split`         | String  | Skip split keybind                 | <kbd>Page Down</kbd>
| `keybind-toggle-decorations` | String  | Toggle window decorations keybind  | <kbd>Right Ctrl</kbd>

### Modifying the default values

There is no settings dialog, but you can change
the values in the `wildmouse.urn` path with `gsettings` or directly
edit them in the [urn-gtk.gschema.xml](https://github.com/leflores-fisi/urn/blob/master/urn-gtk.gschema.xml) file.

You must do `sudo make install` to get the required file `urn-gtk.gschema.xml` into the expected location.

Keybind strings must be parseable by the
[gtk_accelerator_parse](https://docs.gtk.org/gtk4/func.accelerator_parse.html).

## Colors

The color of a time or delta has a special meaning.

| Color       | Meaning                                |
|-------------|----------------------------------------|
| Dark red    | Behind splits in PB and losing time    |
| Light red   | Behind splits in PB and gaining time   |
| Dark green  | Ahead of splits in PB and gaining time |
| Light green | Ahead of splits in PB and losing time  |
| Blue        | Best split time in any run             |
| Gold        | Best segment time in any run           |

## Split Files

Split files are stored as well-formed JSON and **must** contain
one [main object](#main-object). All the keys are optional.

You can use [`splits/sotn.json`](https://github.com/leflores-fisi/urn/blob/master/splits/sotn.json)
as an example or create your own split files and place them
wherever you want.

### Main object

| Key           | Value                                  |
|---------------|----------------------------------------|
| `title`         | Title string at top of window          |
| `attempt_count` | Number of attempts                     |
| `start_delay`   | Non-negative delay until timer starts  |
| `world_record`  | Best known time                        |
| `splits`        | Array of [split objects](#split-object)|
| `theme`         | Window theme                           |
| `theme_variant` | Window theme variant                   |
| `width`         | Window width                           |
| `height`        | Window height                          |

### Split object

| Key          | Value                    |
|--------------|--------------------------|
| `title`        | Split title            |
| `time`         | Split time             |
| `best_time`    | Your best split time   |
| `best_segment` | Your best segment time |

Times are strings in `HH:MM:SS.mmmmmm` format.

## Themes

Create a theme stylesheet and place it
in `themes/<name>/<name>.css` or directly in `~/.urn/themes/<name>/<name>.css`, where `name` is the name of your theme.

You can set the global theme by
changing the `theme` value in gsettings. Theme variants
should follow the pattern `<name>-<variant>.css`.
Your splits can apply their own themes by specifying
a `theme` key in the main object.

See [this project](https://github.com/TheBlackParrot/urn-themes) for
a list of ready-to-use themes.

See the [GtkCssProvider](https://docs.gtk.org/gtk3/class.CssProvider.html) docs for a list of supported CSS properties.

| Urn CSS classes         |
|-------------------------|
| `.window`                 |
| `.header`                 |
| `.title`                  |
| `.attempt-count`          |
| `.time`                   |
| `.delta`                  |
| `.timer`                  |
| `.timer-seconds`          |
| `.timer-millis`           |
| `.delay`                  |
| `.splits`                 |
| `.split`                  |
| `.current-split`          |
| `.split-title`            |
| `.split-time`             |
| `.split-delta`            | 
| `.split-last`             |
| `.done`                   |
| `.behind`                 |
| `.losing`                 |
| `.best-segment`           |
| `.best-split`             |
| `.footer`                 |
| `.prev-segment-label`     |
| `.prev-segment`           |
| `.sum-of-bests-label`     |
| `.sum-of-bests`           |
| `.personal-best-label`    |
| `.personal-best`          |
| `.world-record-label`     |
| `.world-record`           |

If a split has a `title` key, its UI element receives a class
name derived from its title. Specifically, the title is lowercased
and all non-alphanumeric characters are replaced with hyphens, and
the result is concatenated with `split-title-`. For instance,
if your split is titled "First split", it can be styled by
targeting the CSS class `.split-title-first-split`.

---

## FAQ
- How to resize the window application?

    Edit the width and height properties in the [split json file](#main-object).

- How to change the default keybinds?

    For simplicity, you can edit the `urn-gtk.gschema.xml` file. See [Settings and keybinds](#modifying-the-default-values).

- How to make the keybinds global

    Set the `global-hotkeys` property as true. See [Settings and Keybinds](#settings-and-keybinds).

- Can I modify the Urn appareance?

    Yes. You can modify the default CSS theme (`themes/live-split`), download
    [online themes](https://github.com/TheBlackParrot/urn-themes),
    or [make your own theme](#themes).

- How to make my own split file?

    You can use `splits/sotn.json` as an example. You can place the
    split files wherever you want, just open them when starting Urn.

- Can I contribute?

    Yes, you can contribute by making [pull requests](https://github.com/leflores-fisi/urn/pulls) or
    [reporting issues](https://github.com/leflores-fisi/urn/issues). ✨
---

## Uninstall Urn
Uninstall the desktop application by running
```sh
sudo make uninstall
```
