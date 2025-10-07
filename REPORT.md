# Feature 3 — Column Display (ls-v1.2.0)

## General logic for "down then across" columnar printing
To print items in a "down then across" format we:
1. Gather all filenames into an array and determine the longest filename length.
2. Query the terminal width (via `ioctl(TIOCGWINSZ)`) and compute the column width as `max_filename_length + spacing`.
3. Compute the number of columns that fit: `cols = term_width / column_width` (minimum 1).
4. Compute rows needed: `rows = ceil(num_items / cols)`.
5. Print row by row. For row `r`, print items:
   - `names[r]`, `names[r + rows]`, `names[r + 2*rows]`, ..., until columns exhausted.
This layout fills columns top-to-bottom first, then left-to-right across columns. A single linear loop over filenames cannot produce this layout because the physical printed order is not the array order — it requires index arithmetic mapping rows and columns.

## Why a simple single loop is insufficient
A single loop iterating filenames in natural array order prints them left-to-right, top-to-bottom, producing a different visual layout (items appear across rows before moving down). The "down then across" pattern needs items grouped into `rows` slices per column, so each printed row must access non-contiguous elements from the filename array (element indices differ by `rows`). Hence we iterate rows and inside that iterate columns and compute `index = column * rows + row`.

## Purpose of `ioctl(TIOCGWINSZ)` in this context
`ioctl` with the `TIOCGWINSZ` request retrieves the current terminal window size (number of columns). This allows the program to compute how many columns of filenames fit in the user's terminal and dynamically adapt output to the user's environment. Without it, if we used a fixed width (such as 80 columns), the output may:
- Wrap incorrectly on narrow terminals, producing misaligned output.
- Underuse wide terminals (excessively narrow layout) and not present as many columns as possible.
Therefore runtime detection with `ioctl` produces the expected adaptive behavior similar to the standard `ls` utility.

## Limitations of fixed-width fallback
A fixed width (e.g., 80) can serve as a fallback if `ioctl` fails (e.g., not connected to a tty), but it reduces UX:
- On terminals narrower than 80, columns may overflow; long filenames could break layout.
- On very wide terminals, we won't leverage extra space to show more columns.
Thus, while acceptable as a fallback, it is inferior to detecting terminal size at runtime.

