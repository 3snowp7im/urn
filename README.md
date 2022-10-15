# Urn split tracker
![Urn](http://i.imgur.com/1ivi9EZ.png)

## About

This simple split tracker was hacked together by [3snow p7im](https://github.com/3snowp7im).
It was originally written because there were no existing
solutions for split tracking with a delayed start available
on *nix platforms.

Forked from the original project (rest in peace), the goal of
this fork is to add some minor fixes and implement its pull requests.

## Quick start

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

## Usage

When you start the **Urn** application, a file dialog will appear to select
a Split JSON file (see [Split files](#split-files)).

Initially, the window is undecorated. You can toggle window decorations
by pressing the `Right Control` key.


The timer is controlled by key presses:

| Key        | Stopped | Started |
|------------|---------|---------|
| Spacebar   | Start   | Split   |
| Backspace  | Reset   | Stop    |
| Delete     | Cancel  | -       |

Cancel will reset the timer and decrement the attempt counter.
A run that is reset before the start delay is automatically
canceled. If you forgot to split, or accidentally split twice,
you can manually change the current split:

| Key       | Action      |
|-----------|-------------|
| Page Up   | Unsplit     |
| Page Down | Skip split  |

Keybinds can be configured by changing your gsettings.

## Settings 

There is no settings dialog, but you can change
the values in the `wildmouse.urn` path with `gsettings` or directly
change them in the `urn-gtk.gschema.xml` file.

| Key                        | Type    | Description                        |
|----------------------------|---------|------------------------------------|
| start-decorated            | Boolean | Start with window decorations      |
| start-on-top               | Boolean | Start with window as always on top |
| hide-cursor                | Boolean | Hide cursor in window              |
| global-hotkeys             | Boolean | Enables global hotkeys             |
| theme                      | String  | Default theme name                 |
| theme-variant              | String  | Default theme variant              |
| keybind-start-split        | String  | Start/split keybind                |
| keybind-stop-reset         | String  | Stop/Reset keybind                 |
| keybind-cancel             | String  | Cancel keybind                     |
| keybind-unsplit            | String  | Unsplit keybind                    |
| keybind-skip-split         | String  | Skip split keybind                 |
| keybind-toggle-decorations | String  | Toggle window decorations keybind  |

Keybind strings should be parseable by
[gtk_accelerator_parse](https://developer.gnome.org/gtk3/stable/gtk3-Keyboard-Accelerators.html#gtk-accelerator-parse).

## Color Key

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

| Key           | Value                                 |
|---------------|---------------------------------------|
| title         | Title string at top of window         |
| attempt_count | Number of attempts                    |
| start_delay   | Non-negative delay until timer starts |
| world_record  | Best known time                       |
| splits        | Array of split objects                |
| theme         | Window theme                          |
| theme_variant | Window theme variant                  |
| width         | Window width                          |
| height        | Window height                         |

### Split object

| Key          | Value                  |
|--------------|------------------------|
| title        | Split title            |
| time         | Split time             |
| best_time    | Your best split time   |
| best_segment | Your best segment time |

Times are strings in HH:MM:SS.mmmmmm format

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

For a list of supported CSS properties, see
[GtkCssProvider](https://developer.gnome.org/gtk3/stable/GtkCssProvider.html).

| Urn CSS classes         |
|-------------------------|
| .window                 |
| .header                 |
| .title                  |
| .attempt-count          |
| .time                   |
| .delta                  |
| .timer                  |
| .timer-seconds          |
| .timer-millis           |
| .delay                  |
| .splits                 |
| .split                  |
| .current-split          |
| .split-title            |
| .split-time             |
| .split-delta            | 
| .split-last             |
| .done                   |
| .behind                 |
| .losing                 |
| .best-segment           |
| .best-split             |
| .footer                 |
| .prev-segment-label     |
| .prev-segment           |
| .sum-of-bests-label     |
| .sum-of-bests           |
| .personal-best-label    |
| .personal-best          |
| .world-record-label     |
| .world-record           |

If a split has a `title` key, its UI element receives a class
name derived from its title. Specifically, the title is lowercased
and all non-alphanumeric characters are replaced with hyphens, and
the result is concatenated with `split-title-`. For instance,
if your split is titled "First split", it can be styled by
targeting the CSS class `.split-title-first-split`.

## Uninstall Urn
Uninstall the desktop application by running
```sh
sudo make uninstall
```
