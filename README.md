![Urn](http://i.imgur.com/9rTgllV.png)

# About

This simple split tracker was hacked together by 3snow p7im.
It was originally written because there were no exisiting
solutions for split tracking with a delayed start available
on *nix platforms.

# Usage

| Key       | Stopped     | Started    |
|-----------|-------------|------------|
| Spacebar  | Start timer | Split time |
| Backspace | Reset timer | Stop timer |

If you forgot to split, or accidentally split twice, you can
manually change the current split:

| Key       | Action      |
|-----------|-------------|
| Page Up   | Unsplit     |
| Page Down | Skip split  |

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
I your split file is ```game.json```, the stylesheet
should be named ```game.css```.

| Class          | Description                 |
|----------------|-----------------------------|
| .window        | The main timer window       |
| .title         | Title string                |
| .timer         | The current running time    |
| .timer-millis  | Millis part of running time |
| .delay         | Negative running time value |
| .splits        | Split list container        |
| .split         | Split list item             |
| .current-split | Current split               |
| .split-icon    | Split icon box              |
| .split-title   | Split title string          |
| .split-time    | Split time                  |
| .split-delta   | Split time delta            |
| .done          | Finished splits             |
| .behind        | Behind time                 |
| .losing        | Losing time                 |
| .best-segment  | Best segment time           |
| .best-split    | Best split time             |
| .footer        | Window footer               |

If a split has a ```title``` key, its UI element receives a class
name derived from its title. Specifically, the title is lowercased
and all non-alphanumeric characters are replaced with hyphens, and
the result is concatenated with ```split-```. For instance, if
your split is titled "First split", it can be styled by targeting
the CSS class ```.split-first-split```.

Split icon boxes have a default size of 0x0 pixels. If you want
to give them a ```background``` property, you will need to also
size the ```.split-icon``` class (giving it a ```padding```
property works for this).