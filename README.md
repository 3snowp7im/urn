![Urn](http://i.imgur.com/19pQrjP.png)

# About

This simple split tracker was hacked together by 3snow p7im.
It was originally written because there were no exisiting
solutions for split tracking with a delayed start available
on *nix platforms.

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

# File format

* Stored as well-formed json
* Must contain one main object
* All keys are optional
* Times are strings in HH:MM:SS.mmmmmm format

## Main object

| Key          | Value                                 |
|--------------|---------------------------------------|
| title        | Title string at top of window         |
| start_delay  | Non-negative delay until timer starts |
| world_record | Best known time                       |
| splits       | Array of split objects                |
| width        | Window width                          |
| height       | Window height                         |

## Split object

| Key          | Value                  |
|--------------|------------------------|
| title        | Split title            |
| time         | Split time             |
| best_time    | Your best split time   |
| best_segment | Your best segment time |

# Styled elements

You can style the window by creating a ```.css``` file
with the same name as your splits in the same directory.
If your split file is ```game.json```, the stylesheet
should be named ```game.css```.

| Class                   |
|-------------------------|
| .window                 |
| .header                 |
| .title                  |
| .attempt-count          |
| .timer                  |
| .timer-millis           |
| .delta                  |
| .delay                  |
| .splits                 |
| .split                  |
| .current-split          |
| .split-title            |
| .split-time             |
| .split-delta            | 
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
the result is concatenated with ```split-```. For instance, if
your split is titled "First split", it can be styled by targeting
the CSS class ```.split-first-split```.
