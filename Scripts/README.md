# Hold-to-activate cylinder and Arduino bridge

The project is an Unreal Engine 5.5 Blueprint project. The intended gameplay wiring is in `BP_MovingCylinder`; the Arduino process is kept separate so the game can run without hardware.

## Arduino bridge

1. Install Python and pyserial: `py -m pip install pyserial`.
2. Start the bridge before the game, replacing `COM3` with the Arduino's port:

   ```powershell
   py Scripts/arduino_hold_bridge.py --port COM3 --baud 115200
   ```

3. The Arduino sketch should read newline-terminated serial commands at the same baud rate. By default, `1` means active and `0` means inactive. Change `--active-command` and `--inactive-command` to match the sketch.
4. A game client connects to `127.0.0.1:8765` and sends `HOLD\n` while E is down, then `RELEASE\n` once on release. Closing the game connection also sends the inactive command. This bridge is local-only by default.

The bridge does not launch Python itself. Unreal Blueprints need a TCP socket sender (for example, a project-approved Blueprint TCP socket plugin) to connect and send the two messages. A Blueprint's built-in `Event BeginPlay` graph cannot directly send TCP without such a socket node/plugin or a native C++ module.

## Blueprint graph for `BP_MovingCylinder`

Use the cylinder's mesh component as the moving component, and expose `TravelDistance` (cm) and `MoveSpeed` (cm/s) as instance-editable variables.

1. On `BeginPlay`, cache the mesh component's relative location as `StartLocation`; set `TargetLocation = StartLocation + (0, 0, -TravelDistance)` (local Z is down), and initialize `bPlayerNearby = false`, `bHolding = false`.
2. Add a sphere collision component around the interaction area. `BeginOverlap` for the player sets `bPlayerNearby = true`; `EndOverlap` sets it false and calls the same stop path as key release.
3. Show a widget with **“Press and hold E to activate”** only while `bPlayerNearby` and not holding. Hide it on begin hold, end hold, or leaving the area. Ensure the player controller has input enabled while in range.
4. On the player's `IA_ActivateMic` (or an E `Input Action`), use **Started** to begin only if nearby and not already holding: set `bHolding = true`, send `HOLD\n` once, and start a looping timeline from 0 to 1. On **Completed** and **Canceled**, call one shared stop event.
5. The timeline's alpha drives `VInterp To` (or a timeline float lerp) between `StartLocation` and `TargetLocation`, applying the result with `SetRelativeLocation`. This moves the cylinder down smoothly while held.
6. The shared stop event first checks `bHolding`, sets it false, sends `RELEASE\n` once, and reverses the timeline toward 0. The cylinder returns to its cached start location. Leaving the trigger must invoke this event too, so the hardware cannot remain active if the player walks away.

Use Enhanced Input **Started/Completed/Canceled** so release is detected even when E is held across frames. Do not use `Triggered` to send repeated serial commands; only send on the press and release edges.
