# v0.7.1 test run

Close Cry of Fear on both PCs. Every player runs "Install.cmd", then "Run Co-op Mod.cmd".

1. Both players, on a map where you have the knife and the phone in quick slots 1 and 2: press 1 and 2 together. You hold the knife with the phone in the left hand. The host log has `DUAL quick player=N keys ... asked`, with N = 2 for the joining player.
2. Press 1, wait a second, press 2: a plain weapon switch, no dual wield.
3. Switch quickly from 1 to 2 as in a fight. If it dual wields by accident, send the host log; the `DUAL quick` lines show the gap and the limit.
4. On a ladder or in a phone call: 1+2 does nothing.
5. In a long cutscene with several camera shots (the first scene on s4_start), press R late in the scene, then right at a change of shot. With two players, press R at different moments: the count stays, and the scene ends at N/N. The prompt is gone once the scene ends.
6. R on the scenes with a model camera: the Doctor's stomp on s4_attic (R during it, and once during the scene before it: the stomp must not play after), the train scene on s4_subway2st3 after 7 s (you end up back on the map), the endings.
7. After a skip the story goes on as usual (lever doors stay open as long as they should).
8. The prologue's "Press R to skip" stays up. The wardrobe (clothes) shows no prompt.
9. The menu credit says v0.7.1.
