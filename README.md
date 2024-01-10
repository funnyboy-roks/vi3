# vi3

A program to generate vi-style keystrokes for i3 using modes.

## Usage

To run the program just run `./vi3 <input-file> <output-file>`.

Both `input-file` and `output-file` can be substituted with `-` to use
STDIN and STDOUT respectively.

## Example

Input file:

```cfg
exit Escape
exit Ctrl+j
; volume up
v,u exec --no-startup-id pactl set-sink-volume @DEFAULT_SINK@ +5%
; volume down
v,d exec --no-startup-id pactl set-sink-volume @DEFAULT_SINK@ -5%
; volume mute
v,m exec exec --no-startup-id pactl set-sink-mute @DEFAULT_SINK@ toggle
```

Output:

```i3config
mode "Vim: " {
    bindsym v mode "Vim: v"
    bindsym Escape mode "default"
    bindsym Ctrl+j mode "default"
}
mode "Vim: v" {
    bindsym u exec --no-startup-id pactl set-sink-volume @DEFAULT_SINK@ +5%; mode "default"
    bindsym d exec --no-startup-id pactl set-sink-volume @DEFAULT_SINK@ -5%; mode "default"
    bindsym m exec exec --no-startup-id pactl set-sink-mute @DEFAULT_SINK@ toggle; mode "default"
    bindsym Escape mode "default"
    bindsym Ctrl+j mode "default"
}
```

## Building

This project uses libc and no other dependencies.

To build, just run `make` in the root directory. (or just use your C
compiler to compile the `main.c` file)
