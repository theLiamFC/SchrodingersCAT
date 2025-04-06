# Schrödinger's CAT: CRATE Acquisition & Transportation

Schrödinger's CAT is an autonomous robot designed for Stanford's ME218B course and the winner of the 2025 ME218B CARRYER Tournament. The robot's mission is to transport lightweight CRATEs (Container Really Attracted To Electromagnetism) to designated STACKs (Station That Accumulates CRATEs Keenly) in a 1v1 robotics competition.

## Project Overview

- **Objective**: Score points by stacking CRATEs on STACKs within a 244 cm x 244 cm arena.
- **Constraints**:
  - Initial size: 32 cm cube (expandable to 48 cm).
  - Budget: $200.
- **Match Rules**:
  - Robots operate autonomously for a duration of **2 minutes and 18 seconds**.
  - Robots must stay on their assigned side of the arena.

## Key Features

- Hierarchical state machine and event based programming on two PIC32 microcontrollers.
- IR beacon sensing for arena-side identification.
- Auto calibrating 5 sensor line following.
- 2DOF servo arm for CRATE stacking.

## Acknowledgments

This project was developed as part of Stanford University's ME218B course by Liam Campbell, Shiley Einav, Ben Stettin, and Nick Woehrle. For more details, visit the [project page](https://sites.google.com/stanford.edu/schrodingers-cat/home).
