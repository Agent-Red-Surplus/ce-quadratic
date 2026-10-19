# Algebra Suite for TI-84 Plus CE

A native C calculator application being developed as an Honors Algebra 2 toolkit.
It uses a dedicated full-screen interface, exact-answer formatting, and a menu-based workflow.

## Status

Version 0.3.0 adds a quadratic-details screen to the existing solver.
The prior menu/solver version was reported working on the project owner's physical calculator.
This new revision still requires GitHub compilation and physical-calculator testing.
The application does not yet cover a complete Honors Algebra 2 course.

## Implemented in this source

- Quadratic equations in the form `A*x^2 + B*x + C = 0`.
- Linear equations when `A = 0` and `B != 0`.
- Identity and no-solution cases when `A = B = 0`.
- Reduced rational roots and simplified radical roots.
- Nonreal complex roots, including purely imaginary answers without `0+`.
- Repeated-root labeling.
- Stacked fraction bars and drawn radical bars.
- Comma-separated roots, with a second line when needed.
- Main menu and About screen.
- Quadratic details: original discriminant, root classification, exact vertex,
  symmetry axis, and opening direction/minimum or maximum.
- Context-sensitive controls and replace-on-type coefficient editing.

## Supported input and limitations

- Integer coefficients from -1000 through 1000 inclusive.
- Blank coefficient fields mean zero.
- Decimal, fractional, and free-form symbolic coefficient input are not supported.
- Only standard quadratic and linear equations are currently solved.
- Quadratic details apply to the function `y = A*x^2 + B*x + C` with nonzero `A`.
- Higher-degree polynomial solving, general symbolic simplification, and
  completing-the-square steps are not implemented yet.
- The display uses custom graphics, not the OS's native MathPrint engine.
- Unsupported answer width is reported rather than intentionally drawn off-screen.
- No persistent logging, saved inputs, network access, or file-I/O library calls.
- Coefficients remain while navigating within one application session and reset after exit.

## Controls

### Main menu

| Key | Action |
| --- | --- |
| Up / Down | Select item |
| ENTER | Open item |
| CLEAR or Y= | Exit application |

### Coefficient entry

| Key | Action |
| --- | --- |
| Up / Down | Select A, B, or C |
| Digits | Replace a newly selected value; subsequent digits append |
| ENTER | Advance to next field; solve from C |
| GRAPH | Solve from any field |
| DEL | Erase last character and enter append-edit mode |
| CLEAR | Empty current field; an empty field evaluates as zero |
| (-) or subtraction | Toggle sign and enter append-edit mode |
| Y= | Return to main menu |

To replace an existing value with a negative number, type the new digits and
then toggle the sign. Alternatively, CLEAR the field before entering the sign
and digits. An underline indicates replace-on-type mode.

### Results

| Key | Action |
| --- | --- |
| ENTER or CLEAR | Edit coefficients |
| GRAPH | Show equation details |
| Y= | Return to main menu |

### Equation details

| Key | Action |
| --- | --- |
| ENTER, CLEAR, or GRAPH | Return to roots |
| Y= | Return to main menu |

## Calculator requirements

- TI-84 Plus CE with a compatible native-program launcher.
- Development has been tested by the project owner using OS 5.8.5.0074 and Cesium.
- The calculator-side `graphx` library and any runtime support requested by the
  executable must be available. Keep shared library files used by other programs.
- This source does not directly use `fileioc` or `keypadc`.

Launch `ALGEBRA` through the known-working Cesium setup. Do not change the OS,
reset memory, or remove launcher files as a routine installation step.
Back up important calculator files before running development builds.

## Repository layout

```text
README.md
makefile
src/
  main.c
.github/
  workflows/
    build.yml
```

The makefile's calculator program name is `ALGEBRA`.
Do not rename C source to `.8xp`, and do not pass native C source through a
TI-BASIC tokenizer such as SourceCoder.

## Browser-only build and transfer

The existing GitHub Actions workflow downloads CE Toolchain v15.0 onto a remote
Linux runner, runs `make`, and uploads `bin/*.8xp` as a build artifact.
No compiler installation is required on the user's computer.

1. Work on the `feature/algebra-suite` branch.
2. Upload `main.c` inside `src/`, replacing the existing file.
3. Keep `makefile` at the repository root and the workflow in `.github/workflows/`.
4. Commit the changes and open the newest Actions run for the feature branch.
5. After a successful build, download the artifact and extract `ALGEBRA.8xp`.
6. Transfer the compiled file with the existing browser transfer tool.
7. Launch it through Cesium and test on the physical calculator.

The workflow must include `feature/algebra-suite` in its push branch filter.
The saved stable branches should not be used for active development.

A green build verifies successful compilation/linking and artifact creation;
it does not verify calculator execution or correctness of every supported input.

## Verification examples

| A | B | C | Expected roots | D | Vertex | Axis |
| ---: | ---: | ---: | --- | ---: | --- | --- |
| 1 | 3 | -2 | (-3 + sqrt(17))/2, (-3 - sqrt(17))/2 | 17 | (-3/2, -17/4) | x = -3/2 |
| 1 | -2 | 8 | 1 + i*sqrt(7), 1 - i*sqrt(7) | -28 | (1, 7) | x = 1 |
| 1 | 0 | 7 | i*sqrt(7), -i*sqrt(7) | -28 | (0, 7) | x = 0 |
| 1 | 2 | 1 | -1, repeated | 0 | (-1, 0) | x = -1 |
| -1 | 2 | 3 | -1, 3 (either order) | 16 | (1, 4) | x = 1 |
| 0 | 2 | 3 | -3/2 | Not quadratic | None | None |
| 0 | 0 | 0 | All complex numbers | Not quadratic | None | None |
| 0 | 0 | 1 | No solution | Not quadratic | None | None |

README examples use plain-text sqrt notation for portability. Calculator
answers use drawn radical and fraction bars.

For nonzero A, `D = B^2 - 4*A*C`, vertex coordinates are
`h = -B/(2*A)` and `k = -D/(4*A)`. The axis is `x = h`.
The original discriminant need not equal the radicand remaining after
simplification. Vertex and opening information describes the real graph of
the associated quadratic function, even when its roots are nonreal.

## Development roadmap: not yet implemented

- Quadratic derivation/steps, completing the square, and form conversions.
- Complex-number arithmetic and powers of i.
- Polynomial arithmetic, supported factoring, division, and root tools.
- Rational expressions and equations with domain restrictions.
- Radicals, rational exponents, and extraneous-root checks.
- Exponential/logarithmic equations and models.
- Functions, transformations, composition, and supported inverses.
- Linear systems and selected nonlinear systems.
- Sequences and series.
- Trigonometry and unit-circle tools.
- Probability, combinatorics, and selected statistics.

Honors Algebra 2 coverage varies by school. The final checklist should be
compared with the student's actual syllabus. A roadmap item is not a claim
that the current application supports that feature.

## Display and troubleshooting

- Judge graphics on the physical calculator. The browser capture used during
  development produced duplicated/distorted images even when the physical
  display was correct.
- Preserve the default text configuration: a clipped-text change previously
  coincided with unreadable text and was removed in the working build.
- Do not restore diagnostic AppVars or logging code unless deliberately adding
  a separate debugging feature.
- If compilation fails, inspect the first failed build step and its actual error.
- If execution fails, record the launch method and physical-screen behavior.

## Change summary

### 0.3.0

Adds GRAPH-accessible discriminant, classification, exact vertex, symmetry axis,
and opening direction. Guards linear/degenerate cases against quadratic-only
formulas. This revision is pending compilation and calculator testing.

### 0.2.0

Introduced the Algebra Suite menu, integrated the existing exact-root solver,
and retained the log-free controls and rendering approach. Reported working
by the project owner on their calculator.
