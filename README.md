# Solar Panel Solution

See [conference MVP status](CONFERENCE_MVP.md) for the current preview behavior,
build/flash caveats, and super-repo handoff.

## Clone

```bash
cd v6_workspace/

git clone git@github.com:silabs-apnagy/emdemo-solar-app.git MatterSolarPanelSolution
```

## Build
Open the Solution that was cloned into the Studio workspace in Studio 6.
Open the MatterSolarPanelOverThread.slcp
On the Overview panel in the Project Details panel press the 3 dots ... and select Force Generation to be safe. 
The project should build in VS code. 

## Run
Press PB1 to toggle the Solar panel production states.

Current states: Night (0) - Overcast (30-40%) - Clear (100%)
