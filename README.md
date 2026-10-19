# Algebra Suite v0.4.1 — readable worked solutions

This revision replaces the dense v0.4.0 transcript with named, one-action-at-a-time work screens. Each screen has one short instruction and no more than two mathematical lines. Equations are drawn at double text scale when they fit, with stacked fractions and radical bars. Longer expressions use normal scale or a short wrapped fallback.

## Update

Use feature/algebra-suite. Upload the complete src/main.c from this bundle into the repository's src folder, replacing the old file. Upload this README at the root. Keep the existing makefile (NAME = ALGEBRA) and GitHub workflow. Build, download ALGEBRA.8xp, and transfer directly. Preserve your saved working branch.

## Work style

- Start with a clean equation, not a list of internal variables.
- Substitute values, then simplify on the next line.
- Show one task per screen: find D, use the formula, simplify the root, take a plus/minus case, reduce a fraction, or find a vertex coordinate.
- Use one brief explanation instead of repeated programming-style narration.
- Render mathematical fractions and radicals rather than leaving sqrt(...) text everywhere.
- Keep the automatic worked solution and replay through TRACE.

The scope remains integer coefficients A, B, C from -1000 to 1000 in A*x^2+B*x+C=0, including linear/constant cases. It is not yet a general symbolic algebra system or the full Honors Algebra 2 suite. The sum/product check is explained as a root check, not direct substitution into every irrational expression.

## Controls

Main menu: UP/DOWN select, ENTER open, CLEAR or Y= exit.

Input: UP/DOWN select a coefficient, digits replace a newly selected value, DEL backspaces, CLEAR clears the field, negate/subtraction toggles sign, ENTER advances then solves from C, GRAPH solves immediately, Y= returns to menu. To replace an old coefficient with a new negative value, enter the new magnitude and then toggle its sign. Blank fields mean zero.

Work: UP/LEFT previous step; DOWN/RIGHT next step; ENTER next step and then roots; GRAPH or CLEAR roots; Y= main menu.

Roots: ENTER/CLEAR edit, GRAPH details, TRACE replay work, Y= menu.

Details: ENTER/CLEAR/GRAPH roots, TRACE replay work, Y= menu.

No persistent logs or saved inputs. Coefficients and work remain in session RAM only.

## Validation status

The C solver, step builder, and mathematical line parser/drawing paths were run using host-compiler graphics/keyboard stubs with undefined-behavior instrumentation. 3,339 input cases passed root-identity and emitted root-check tests with no work-card overflow. Maximum observed length: 30 work screens. Python also checked the vertex identity for each quadratic input. The host build produced no compiler diagnostics under the configured warning flags.

This is not a CE-toolchain compilation or a physical-calculator display test. Real font sizes, screen spacing, controls, target memory layout, and readability need the GitHub build and calculator check. Review those before calling this module finished.

## Acceptance check

Test (A,B,C): (1,3,-2), (1,-2,8), (-1,2,3), (1,2,1), (1,0,4), (2,0,-1), (0,2,3), (0,0,0), (0,0,1).

Look for readable math, complete fraction bars, no overlapping notes, correct negative signs, correct final roots/vertex, and smooth return navigation. Judge the physical calculator screen; browser captures in this project have appeared duplicated.
