![Urn](http://i.imgur.com/9rTgllV.png)

# About

This simple split tracker was hacked together by 3snow p7im.
It has no frills and is not customizable. It was originally
written because there were no exisiting solutions for split
tracking with a delayed start available on *nix platforms.

# Usage

| Key       | Stopped     | Started    |
|-----------|-------------|------------|
| Spacebar  | Start timer | Split time |
| Backspace | Reset timer | Stop timer |

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
| -------------|---------------------------------------|
| title        | Title string at top of window         |
| start_delay  | Non-negative delay until timer starts |
| world_record | Best known time                       |
| splits       | Array of split objects                |

## Split object

| Key          | Value                                 |
| -------------|---------------------------------------|
| title        | Split title                           |
| time         | Split time                            |
| best_time    | Your best split time                  |
| best_segment | Your best segment time                |
