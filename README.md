![Urn](http://i.imgur.com/T6cknpk.png)

# About

This simple split tracker was hacked together by 3snow p7im.
It was originally written because there were no exisiting
solutions for split tracking with a delayed start available
on *nix platforms.

Urn requires ```libgtk+-3.0```, ```libkeybinder-3.0```
and ```libjansson```.

# Usage

Initially the window is undecorated. You can toggle window decorations
by pressing right ```Control```.

The timer is controlled by key presses:

| Key        | Stopped | Started |
|------------|---------|---------|
| Spacebar   | Start   | Split   |
| Backspace  | Reset   | Stop    |
| Delete     | Cancel  | -       |

Cancel will reset the timer and decrement the attempt counter.
A run that is reset before the start delay is automatically
cancelled. If you forgot to split, or accidentally split twice,
you can manually change the current split:

| Key       | Action      |
|-----------|-------------|
| Page Up   | Unsplit     |
| Page Down | Skip split  |

Keybinds can be configured by changing your gsettings.

# Color Key

The color of a time or delta has special meaning.

| Color       | Meaning                                |
|-------------|----------------------------------------|
| Dark red    | Behind splits in PB and losing time    |
| Light red   | Behind splits in PB and gaining time   |
| Dark green  | Ahead of splits in PB and gaining time |
| Light green | Ahead of splits in PB and losing time  |
| Blue        | Best split time in any run             |
| Gold        | Best segment time in any run           |

# Split Files

* Stored as well-formed json
* Must contain one main object
* All keys are optional
* Times are strings in HH:MM:SS.mmmmmm format

## Main object

| Key           | Value                                 |
|---------------|---------------------------------------|
| title         | Title string at top of window         |
| start_delay   | Non-negative delay until timer starts |
| world_record  | Best known time                       |
| splits        | Array of split objects                |
| theme         | Window theme                          |
| theme_variant | Window theme variant                  |
| width         | Window width                          |
| height        | Window height                         |

## Split object

| Key          | Value                  |
|--------------|------------------------|
| title        | Split title            |
| time         | Split time             |
| best_time    | Your best split time   |
| best_segment | Your best segment time |

## Settings

Currently there is no settings dialog, but you can change
the values in ```wildmouse.urn``` path with ```gsettings```.

| Key                        | Type    | Description                       |
|----------------------------|---------|-----------------------------------|
| start-decorated            | Boolean | Start with window decorations     |
| hide-cursor                | Boolean | Hide cursor in window             |
| global-hotkeys             | Boolean | Enables global hotkeys            |
| theme                      | String  | Default theme name                |
| theme-variant              | String  | Default theme variant             |
| keybind-start-split        | String  | Start/split keybind               |
| keybind-stop-reset         | String  | Stop/Reset keybind                |
| keybind-cancel             | String  | Cancel keybind                    |
| keybind-unsplit            | String  | Unsplit keybind                   |
| keybind-skip-split         | String  | Skip split keybind                |
| keybind-toggle-decorations | String  | Toggle window decorations keybind |

## Themes

Create a theme stylesheet and place it
in ```~/.urn/themes/<name>/<name>.css``` where ```name```
is the name of your theme. You can set the global theme by
changing the ```theme``` value in gsettings. Theme variants
should follow the pattern ```<name>-<variant>.css```.
Your splits can apply their own themes by specifying
a ```theme``` key in the main object. 

| Class                   |
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
| .previous-segment-label |
| .previous-segment       |
| .sum-of-bests-label     |
| .sum-of-bests           |
| .personal-best-label    |
| .personal-best          |
| .world-record-label     |
| .world-record           |

If a split has a ```title``` key, its UI element receives a class
name derived from its title. Specifically, the title is lowercased
and all non-alphanumeric characters are replaced with hyphens, and
the result is concatenated with ```split-title-```. For instance,
if your split is titled "First split", it can be styled by
targeting the CSS class ```.split-title-first-split```.
